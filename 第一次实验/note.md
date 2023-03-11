# 实验笔记

## 任务二

``
nasm -f elf64 -o alu.o alu.asm
ld -o alu alu.o
./alu
``

## 任务一

### 环境

在ubuntu20.04上安装好：bochs；bochsGUI库：bochs-x；nasm。

``
sudo apt-get install bochs
sudo apt-get install bochs-x
sudo apt-get install nasm
``

可用资源有：boot.asm; bochsrc

### 步骤

step1：在共享文件夹启动终端，使用nasm汇编boot.asm生成boot.bin

``
nasm boot.asm -o boot.bin
``

step2：生成虚拟软盘

``
bximage -> fd -> 1.44 -> a.img
``

step3：使用dd命令将boot.bin写入软盘

``
dd if=boot.bin of=a.img bs=512 count=1 conv=notrunc
; if: input file
; of: output file
; bs: block size
; count: block num
; conv=notrunc: no other operations
``

step4：使用bochsrc启动bochs, 黑屏在终端按c

``
bochs -f bochsrc
``
