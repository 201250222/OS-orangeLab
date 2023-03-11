#include "iostream"
#include "string"
#include "vector"
#include <cstring>


#define COMMAND_ERR "Invalid Command!\n"
#define NOT_A_FILE "Not A File!\n"
#define PARAM_WRONG "More Than Two Params In Cat-Param!\n"
#define NO_FILE "No Such File!\n"
#define BAD_CLUSTER "Bad Cluster!\n"
#define LS_A_FILE "Ls-Param Is A File!\n"
#define LS_PATH_MORE "More Than One Path In Ls-Param\n"
#define L_FORMAT_ERR "Wrong -l Param In Ls-Param\n"
#define NO_DIR "No Such Directory!\n"

#define EXIT "Exit.\n"
#define FAT_URL "./FAT12/a.img"
#define DIRECTORY 0x10

using namespace std;

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

extern "C" {
    void print(const char *, const int);
}

void asmPrint(const char *s) {
    print(s, strlen(s));
    // cout << s << endl;
}

int BytsPerSec;				//每扇区字节数
int SecPerClus;				//每簇扇区数
int RsvdSecCnt;				//Boot记录占用的扇区数
int FATcnt;				    //FAT表个数
int RootEntCnt;				//根目录最大文件数
int FATSecCnt;		    	//FAT扇区数
// FAT1的偏移字节
int fatBase;
int fileRootBase;
int dataBase;
int BytsPerClus;
#pragma pack(1) /*指定按1字节对齐*/


// 文件树
class Entry {
    string name;
    vector<Entry*> next;
    string path;
    uint32_t FileSize;
    bool isFile = false;
    bool isVal = true;
    int dir_count = 0;
    int file_count = 0;
    char *content = new char[1000];
public:
    Entry() = default;

    Entry(string name, bool isVal);

    Entry(string name, string path);

    Entry(string name, uint32_t fileSize, bool isFile, string path);

    void setPath(string p) { this->path = p; }

    void setName(string n) { this->name = n; }

    void addChild(Entry *child);

    void addFileChild(Entry *child);

    void addDirChild(Entry *child);

    string getName() { return name; }

    string getPath() { return this->path; }

    char* getContent() { return content; }

    bool getIsFile() { return isFile;}

    vector<Entry*> getNext() { return next; }

    bool getIsVal() { return isVal; }

    uint32_t getFileSize() { return FileSize; }
};

Entry::Entry(string name, bool isVal) {
    this->name = name;
    this->isVal = isVal;
}

Entry::Entry(string name, string path) {
    this->name = name;
    this->path = path;
}

Entry::Entry(string name, uint32_t fileSize, bool isFile, string path) {
    this->name = name;
    this->FileSize = fileSize;
    this->isFile = isFile;
    this->path = path;
}

void Entry::addChild(Entry *child) {
    this->next.push_back(child);
}

void Entry::addFileChild(Entry *child) {
    this->next.push_back(child);
    this->file_count++;
}

void Entry::addDirChild(Entry *child) {
    child->addChild(new Entry(".", false));
    child->addChild(new Entry("..", false));
    this->next.push_back(child);
    this->dir_count++;
}
// Entry Class


class BPB {
    // 顺序存储的25Byte
    uint16_t BPB_BytsPerSec;    //每扇区字节数
    uint8_t BPB_SecPerClus;     //每簇扇区数
    uint16_t BPB_RsvdSecCnt;    //Boot记录占用的扇区数
    uint8_t BPB_FATcnt;    	//FAT表个数
    uint16_t BPB_RootEntCnt;    //根目录最大文件数
    uint16_t BPB_TotSec16;
    uint8_t BPB_Media;
    uint16_t BPB_FATSecCnt16;       //FAT扇区数
    uint16_t BPB_SecPerTrk;
    uint16_t BPB_NumHeads;
    uint32_t BPB_HiddSec;
    uint32_t BPB_TotSec32;      //如果BPB_FATSecCnt16为0，该值为FAT扇区数
public:
    BPB() {};

    void init(FILE *fat12);
};

void BPB::init(FILE *fat12) {
    fseek(fat12, 11, SEEK_SET); //BPB从偏移11个字节处开始
    fread(this, 1, 25, fat12); //BPB长度为25字节，从fat12中读出BPB

    // 初始化与BPB相关的全局变量
    BytsPerSec = this->BPB_BytsPerSec;
    SecPerClus = this->BPB_SecPerClus;
    RsvdSecCnt = this->BPB_RsvdSecCnt;
    FATcnt = this->BPB_FATcnt;
    RootEntCnt = this->BPB_RootEntCnt;
    if (this->BPB_FATSecCnt16 != 0) FATSecCnt = this->BPB_FATSecCnt16;
    else FATSecCnt = this->BPB_TotSec32;
    fatBase = RsvdSecCnt * BytsPerSec;
    fileRootBase = (RsvdSecCnt + FATcnt * FATSecCnt) * BytsPerSec; //根目录首字节的偏移数=boot+fat1&2的总字节数
    dataBase = BytsPerSec * (RsvdSecCnt + FATSecCnt * FATcnt + (RootEntCnt * 32 + BytsPerSec - 1) / BytsPerSec);
    BytsPerClus = SecPerClus * BytsPerSec; //每簇的字节数
}
// BPB Class


// 根目录条目
class RootEntry {
    // RootEntry有32字节
    char DIR_Name[11];
    uint8_t DIR_Attr; //文件属性 DIRECTORY=0x10
    char reserved[10]; //未用到的10字节
    uint16_t DIR_WrtTime;
    uint16_t DIR_WrtDate;
    uint16_t DIR_FstClus; //开始簇号
    uint32_t DIR_FileSize; //文件字节数
public:
    RootEntry() {};

    void initRootEntry(FILE *fat12, Entry *root); //初始化root，获取全部文件

    bool isEmptyName();

    bool isValidNameAt(int i);

    bool isInvalidName();

    bool isFile();

    void generateFileName(char name[12]);

    void generateDirName(char name[12]);

    uint32_t getFileSize() { return DIR_FileSize; }

    uint16_t getFstClus() { return DIR_FstClus; }
    
    char* getName() { return DIR_Name; }
};

/**
 * @brief 获取簇号为num的FAT项的值
 * 
 * @param fat12 
 * @param num 簇号
 * @return int 
 */
int getFATValue(FILE *fat12, int num) {
    int base = RsvdSecCnt * BytsPerSec;
    int pos = base + num * 3 / 2;
    int type = num % 2;

    uint16_t bytes;
    uint16_t *bytesPtr = &bytes;
    fseek(fat12, pos, SEEK_SET);
    fread(bytesPtr, 1, 2, fat12);

    if (type == 0) { bytes = bytes << 4; }
    return bytes >> 4;
}

/**
 * @brief 读取文件类型子节点的内容
 * 
 * @param fat12 
 * @param startClus 开始簇号
 * @param child 该子节点
 */
void RetrieveContent(FILE *fat12, int startClus, Entry *child) {
    // 计算基址
    int base = BytsPerSec * (RsvdSecCnt + FATSecCnt * FATcnt + (RootEntCnt * 32 + BytsPerSec - 1) / BytsPerSec);
    // 簇号
    int currentClus = startClus;
    int cluster = 0;
    char *pointer = child->getContent();

    if (startClus == 0) return;

    while (cluster < 0xFF8) {
        cluster = getFATValue(fat12, currentClus);
        if (cluster == 0xFF7) {
            asmPrint(BAD_CLUSTER);
            break;
        }

        // 大小
        int size = SecPerClus * BytsPerSec;
        char *str = (char*)malloc(size);
        char *content = str;
        // 开始字节
        int startByte = base + (currentClus - 2)*SecPerClus*BytsPerSec;

        fseek(fat12, startByte, SEEK_SET);
        fread(content, 1, size, fat12);

        for (int i = 0; i < size; ++i) {
            *pointer = content[i];
            pointer++;
        }
        free(str);
        currentClus = cluster;
    }
}

/**
 * @brief 子节点是目录则递归地获取该子节点的所有子节点
 * 
 * @param fat12 
 * @param startClus 开始簇号
 * @param root 该子节点
 */
void readChildren(FILE *fat12, int startClus, Entry *root) {

    int base = BytsPerSec * (RsvdSecCnt + FATSecCnt * FATcnt + (RootEntCnt * 32 + BytsPerSec - 1) / BytsPerSec);

    int currentClus = startClus;
    int value = 0;
    while (value < 0xFF8) {
        value = getFATValue(fat12, currentClus);
        if (value == 0xFF7) {
            asmPrint(BAD_CLUSTER);
            break;
        }

        int startByte = base + (currentClus - 2) * SecPerClus * BytsPerSec;

        int size = SecPerClus * BytsPerSec;
        int loop = 0;
        while (loop < size) {
            RootEntry *rootEntry = new RootEntry();
            fseek(fat12, startByte + loop, SEEK_SET);
            fread(rootEntry, 1, 32, fat12);

            loop += 32;

            if (rootEntry->isEmptyName() || rootEntry->isInvalidName()) continue;
            
            char tmpName[12];
            if ((rootEntry->isFile())) {    //file
                rootEntry->generateFileName(tmpName);
                Entry *child = new Entry(tmpName, rootEntry->getFileSize(), true, root->getPath());
                root->addFileChild(child);
                RetrieveContent(fat12, rootEntry->getFstClus(), child);
            } else {                        //directory
                rootEntry->generateDirName(tmpName);
                Entry *child = new Entry();
                child->setName(tmpName);
                child->setPath(root->getPath() + tmpName + "/");
                root->addDirChild(child);
                readChildren(fat12, rootEntry->getFstClus(), child);
            }
        }
    }
}

void RootEntry::initRootEntry(FILE *fat12, Entry *root) {
    int base = fileRootBase;
    char realName[12];


    for (int i = 0; i < RootEntCnt; ++i) {
        fseek(fat12, base, SEEK_SET);
        fread(this, 1, 32, fat12);
        
        base += 32;

        if (isEmptyName() || isInvalidName()) continue;

        if (this->isFile()) {
            generateFileName(realName);
            Entry *child = new Entry(realName, this->DIR_FileSize, true, root->getPath());
            root->addFileChild(child);
            RetrieveContent(fat12, this->DIR_FstClus, child);
        } else {
            generateDirName(realName);
            Entry *child = new Entry();
            child->setName(realName);
            child->setPath(root->getPath() + realName + "/");
            root->addDirChild(child);
            readChildren(fat12, this->getFstClus(), child);
        }
    }
}

bool RootEntry::isValidNameAt(int j) {
    return ((this->DIR_Name[j] >= 'A') && (this->DIR_Name[j] <= 'Z'))
           ||((this->DIR_Name[j] >= '0') && (this->DIR_Name[j] <= '9'))
           ||(this->DIR_Name[j] == ' ');
          // no need of " ||(this->DIR_Name[j] == '.') " cause name has no format
}

bool RootEntry::isInvalidName() {
    int invalid = false;
    for (int k = 0; k < 11; ++k) {
        if (!this->isValidNameAt(k)) {
            invalid = true;
            break;
        }
    }
    return invalid;
}

bool RootEntry::isEmptyName(){
    return this->DIR_Name[0] == '\0';
}

bool RootEntry::isFile(){
    return (this->DIR_Attr & DIRECTORY) == 0;
}

void RootEntry::generateFileName(char name[12]) {
    int tmp = -1;
    for (int j = 0; j < 11; ++j) {
        if (this->DIR_Name[j] != ' ') {
            name[++tmp] = this->DIR_Name[j];
        } else {
            name[++tmp] = '.';
            while (this->DIR_Name[j] == ' ') j++;
            j--;
        }
    }
    name[++tmp] = '\0';
}

void RootEntry::generateDirName(char *name) {
    int tmp = -1;
    for (int k = 0; k < 11; ++k) {
        if (this->DIR_Name[k] != ' ') {
            name[++tmp] = this->DIR_Name[k];
        } else {
            name[++tmp] = '\0';
            break;
        }
    }
}
// RootEntry Class


// 执行指令

// 重载一个类型转换的split，方便处理指令或路径
vector<string> splitCmd(const string& str, const string& delim) {
    vector <string> res;
    if ("" == str) return res;
    //先将要切割的字符串从string类型转换为char*类型
    char *strs = new char[str.length() + 1]; //不要忘了
    strcpy(strs, str.c_str());

    char *d = new char[delim.length() + 1];
    strcpy(d, delim.c_str());

    char *p = strtok(strs, d);
    while (p) {
        string s = p; //分割得到的字符串转换为string类型
        res.push_back(s); //存入结果数组
        p = strtok(NULL, d);
    }

    return res;
}

/**
 * @brief 格式化指令中的路径
 * 
 * @param s 路径参数
 */
void formatPath(string &s) { if (s[0] != '/') { s = "/" + s; } }

/**
 *
 * @brief  /./  or  /../
 *
 */
string calPath(string &s) {
    string path = "";
    string *tmp = new string[20];
    int count = 0;
    vector<string> names = splitCmd(s, "/");
    for(int i=0;i<names.size();++i){
        if(names[i] == "."){
            //11.12 if(count>0) tmp[--count] = "";
        }else if(names[i] == ".."){
            /*11.12
            if(count>1){
                tmp[--count] = "";
                tmp[--count] = "";
            }else if(count == 1) tmp[--count] = "";
            */
            if(count >= 1) tmp[--count] = "";
        }else{
            tmp[count++] = names[i];
        }
    }
    if(count <= 0) path = "/";
    else{
        for(int i=0;i<count;++i){
            path = path + "/" + tmp[i];
        }
    }
    return path;
}

/**
 * @brief 递归地寻找用户cat的文件并打印
 * 
 * @param root 当前的根目录
 * @param p 用户输入的路径
 */
void printCAT(Entry *root, string p) {
    formatPath(p);
    if (p == root->getPath() + root->getName()) {
        if (root->getIsFile()) {
            if (root->getContent()[0] != 0) {
                asmPrint(root->getContent());
                asmPrint("\n");
            }
            return;
        } else {
            asmPrint(NOT_A_FILE);
            return;
        }

    }
    if (p.length() <= root->getPath().length()) {
        asmPrint(NO_FILE);
        return;
    }
    string tmp = p.substr(0, root->getPath().length());
    if (tmp == root->getPath()) {
        for (Entry *q : root->getNext()) {
            printCAT(q, p);
        }
    }
}

void CAT(vector<string> commands, Entry* root) {
    if (commands.size() == 2 && commands[1][0] != '-') {
        printCAT(root, calPath(commands[1]));
    } else {
        asmPrint(PARAM_WRONG);
    }
}

/**
 * @brief ls path:递归地打印r节点的所有子节点
 * @todo 修改输出
 * 
 * @param r path中的最后一级节点
 */
void printLS(Entry *r) {
    string str;
    Entry *p = r;
    if (p->getIsFile()) return;
    else {
        str = p->getPath() + ":\n";
        const char *strPtr = str.c_str();
        asmPrint(strPtr);
        str.clear();
        //打印每个next
        Entry *q;
        int len = p->getNext().size();
        for (int i = 0; i < len; i++) {
            q = p->getNext()[i];
            if (!q->getIsFile()) {
                string tmp_str = "\033[31m" + q->getName() + "\033[0m" + "  ";
                const char *tmp_ptr = tmp_str.c_str();
                asmPrint(tmp_ptr);
            } else {
                string tmp_str = q->getName() + " ";
                const char *tmp_ptr = tmp_str.c_str();
                asmPrint(tmp_ptr);
            }
        }
        asmPrint("\n");
        for (int i = 0; i < len; ++i) {
            if (p->getNext()[i]->getIsVal()) {
                printLS(p->getNext()[i]);
            }
        }
    }
}

void printLSL(Entry *r) {
    string str;
    Entry *p = r;
    if (p->getIsFile()) return;
    else {
        // 计算子文件数和子文件夹数
        int fileNum = 0;
        int dirNum = 0;
        for (int j = 0; j < p->getNext().size(); ++j) {
            if (p->getNext()[j]->getName() == "." || p->getNext()[j]->getName() == "..") {
                continue;
            }
            if (p->getNext()[j]->getIsFile()) {
                fileNum++;
            } else {
                dirNum++;
            }
        }
        //先打印该节点
        string tmp_str = p->getPath() + " " + to_string(dirNum) + " " + to_string(fileNum) + ":\n";
        const char* tmp_ptr = tmp_str.c_str();
        asmPrint(tmp_ptr);
        str.clear();
        //打印子节点
        Entry *q;
        int len = p->getNext().size();
        for (int i = 0; i < len; i++) {
            q = p->getNext()[i];
            if (!q->getIsFile()) {
                if (q->getName() == "." || q->getName() == "..") {
                    // 打印. ..
                    string tmp_str = "\033[31m" + q->getName() + "\033[0m" + "  \n";
                    const char *tmp_ptr = tmp_str.c_str();
                    asmPrint(tmp_ptr);
                } else {
                    fileNum = 0;
                    dirNum = 0;
                    for (int j = 2; j < q->getNext().size(); ++j) {
                        if (q->getNext()[j]->getIsFile()) {
                            fileNum++;
                        } else {
                            dirNum++;
                        }
                    }
                    string tmp_str = "\033[31m" + q->getName() + "\033[0m" + " " + to_string(dirNum) + " " + to_string(fileNum) + ":\n";
                    const char* tmp_ptr = tmp_str.c_str();
                    asmPrint(tmp_ptr);
                }
            } else {
                string tmp_str = q->getName() + " " + to_string(q->getFileSize()) + "\n";
                const char* tmp_ptr = tmp_str.c_str();
                asmPrint(tmp_ptr);
            }
        }
        asmPrint("\n");
        for (int i = 0; i < len; ++i) {
            if (p->getNext()[i]->getIsVal()) {
                printLSL(p->getNext()[i]);
            }
        }
    }
}

/**
 * @brief 判断-l参数的格式是否正确
 * 
 */
bool isL(vector<string> commands){
    bool lFormat = true;
    int i = 1;
    for(string cmd : commands){
        if(cmd[0]=='-'){
            for(i=1;i<cmd.length();++i){
                if(cmd[i] != 'l'){
                    lFormat = false;
                }
            }
        }
    }
    return lFormat;
}

/**
 * @brief 计算ls指令中的路径参数个数
 * 
 */
int countPath(vector<string> commands){
    int count = 0;
    for(string cmd : commands){
        if(cmd[0] != '-') ++count;
    }
    return count - 1;
}

/**
 * @brief 找到ls指令path的根节点
 * 
 * @param root 
 * @param dirs 
 * @return Entry* 
 */
Entry* findLsRoot(Entry *root, vector<string> dirs) {
    if (dirs.empty()) return root;
    string name = dirs[dirs.size()-1];
    for (int i=0; i<root->getNext().size(); i++) {
        if (name == root->getNext()[i]->getName()) {
            dirs.pop_back();
            return findLsRoot(root->getNext()[i], dirs);
        }
    }
    return nullptr;
}

/**
 * @brief 处理路径参数
 * 
 */
Entry* dealPath(string &s, Entry *root) {
    formatPath(s);
    vector<string> tmp = splitCmd(s, "/");
    vector<string> dirs;
    while (!tmp.empty()) {
        dirs.push_back(tmp[tmp.size()-1]);
        tmp.pop_back();
    }
    return findLsRoot(root, dirs);
}

/**
 * @brief 在输入指令中找到路径参数下标
 * 
 */
int findPath(vector<string> commands){
    for(int i=1;i<commands.size();++i){
        if(commands[i][0]!='-') return i;
    }
    return -1;
}

void LS(vector<string> commands, Entry* root) {
    if (commands.size() == 1) { // ls
        printLS(root);
    }
    else if(commands.size() == 2){ // ls -l 或者 ls 路径
        // if(commands[1] == "-l"||commands[1] == "-ll") { printLSL(root); }
        // if(isL(commands)) { printLSL(root); }
        if(commands[1][0] == '-'){
            bool flag = true;
            for(int i=1;i<commands[1].length();++i){
                if(commands[1][i] != 'l') flag = false;
            }
            if(flag) printLSL(root);
            else asmPrint(L_FORMAT_ERR);
        }
        else{
            string path = calPath(commands[1]);
            Entry* lsRoot = dealPath(path, root);
            if(lsRoot == nullptr) asmPrint(NO_DIR);
            else if( lsRoot->getIsFile() ) asmPrint(LS_A_FILE);
            else printLS(lsRoot);
        }
    }
    else{  // ls -l 但是有超过一个参数
        // 超过一个路径
        int pathNum = countPath(commands);
        if(pathNum>1) asmPrint(LS_PATH_MORE);
        // -l 语法错误
        else if(!isL(commands)) asmPrint(L_FORMAT_ERR);
        else if(pathNum == 0) printLSL(root);
        else{
            // 正确
            // cout << "[DEBUG]: into -l handler" << endl;
            int pathIndex = findPath(commands);
            // cout << "[DEBUG]: get path index: " << pathIndex << endl;
            // cout << "[DEBUG]: string at path index is " << commands[pathIndex] << endl;
            string path = calPath(commands[pathIndex]);
            // cout << "[DEBUG]: -l path is " << path << endl;
            Entry* lslRoot = dealPath(path, root);
            if(lslRoot == nullptr) asmPrint(NO_DIR);
            else if( lslRoot->getIsFile() ) asmPrint(LS_A_FILE);
            else printLSL(lslRoot);
        }
    }
}


int main() {
    // 打开FAT12的映像文件
    FILE *fat12;
    fat12 = fopen(FAT_URL, "rb");
    // 获取BPB
    BPB *bpb = new BPB();
    bpb->init(fat12);
    // 创建root节点
    Entry *root = new Entry();
    root->setName("");
    root->setPath("/");
    // 获取根目录
    RootEntry *rootEntry = new RootEntry();
    rootEntry->initRootEntry(fat12, root);

    while (true) {
        asmPrint(">");
        string input;
        getline(cin, input);
        vector<string> commands = splitCmd(input, " ");
        if (commands[0] == "exit") {
            asmPrint(EXIT);
            fclose(fat12);
            break;
        } else if (commands[0] == "cat") {
            CAT(commands, root);
        } else if (commands[0] == "ls") {
            LS(commands, root);
        } else {
            asmPrint(COMMAND_ERR);
            continue;
        }
    }

}

