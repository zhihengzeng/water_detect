#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
STM32 Flash/RAM 使用情况分析脚本
用法: python memory_usage.py <elf文件路径>
"""

import os
import sys
import subprocess
import re
from pathlib import Path

def parse_memory_from_ld(ld_file_path):
    flash_size = 0
    ram_size = 0
    
    # 获取当前脚本的目录
    current_dir = os.path.dirname(os.path.abspath(__file__))
    # 构建完整的文件路径
    full_path = os.path.join(os.path.dirname(current_dir), 'STM32F407XX_FLASH.ld')
    
    try:
        with open(full_path, 'r') as f:
            content = f.read()
            # 查找 MEMORY 块
            if 'MEMORY' in content:
                # 解析 FLASH
                if 'FLASH (rx)' in content:
                    flash_line = content.split('FLASH (rx)')[1].split('\n')[0]
                    flash_length = flash_line.split('LENGTH = ')[1].split('K')[0]
                    flash_size = int(flash_length)
                
                # 解析 RAM
                if 'RAM (xrw)' in content:
                    ram_line = content.split('RAM (xrw)')[1].split('\n')[0]
                    ram_length = ram_line.split('LENGTH = ')[1].split('K')[0]
                    ram_size = int(ram_length)

                # 解析 CCRAM
                if 'CCMRAM (xrw)' in content:
                    ccmram_line = content.split('CCMRAM (xrw)')[1].split('\n')[0]
                    ccmram_length = ccmram_line.split('LENGTH = ')[1].split('K')[0]
                    ccmram_size = int(ccmram_length)
    except FileNotFoundError:
        print(f"警告: 找不到文件 {full_path}")
        print("使用默认值")
    except Exception as e:
        print(f"发生错误: {e}")
        print("使用默认值")
    
    return flash_size, ram_size + ccmram_size

# 读取 ld 文件中的内存大小
FLASH_SIZE, RAM_SIZE = parse_memory_from_ld('../STM32F407XX_FLASH.ld')

# 如果解析失败，使用默认值
if FLASH_SIZE == 0:
    FLASH_SIZE = 1024  # 默认 1024K
if RAM_SIZE == 0:
    RAM_SIZE = 128    # 默认 128K

print(f"FLASH 大小: {FLASH_SIZE}K")
print(f"RAM 大小: {RAM_SIZE}K")

def get_size_output(elf_file):
    """运行 arm-none-eabi-size 命令获取内存使用情况"""
    try:
        # 尝试查找 arm-none-eabi-size 工具
        size_cmd = "arm-none-eabi-size"
        result = subprocess.run([size_cmd, elf_file], 
                               stdout=subprocess.PIPE, 
                               stderr=subprocess.PIPE,
                               text=True)
        if result.returncode != 0:
            print(f"错误: 无法运行 {size_cmd}")
            print(result.stderr)
            sys.exit(1)
        return result.stdout
    except FileNotFoundError:
        print("错误: 未找到 arm-none-eabi-size 工具")
        print("请确保 GNU Arm Embedded Toolchain 已安装并添加到 PATH 中")
        sys.exit(1)

def parse_size_output(output):
    """解析 size 命令的输出"""
    lines = output.strip().split('\n')
    if len(lines) < 2:
        print("错误: size 命令输出格式不正确")
        sys.exit(1)
    
    # 解析第二行，包含实际的大小数据
    data = lines[1].split()
    if len(data) < 4:
        print("错误: size 命令输出格式不正确")
        sys.exit(1)
    
    text = int(data[0])  # .text 段 (代码)
    data_size = int(data[1])  # .data 段 (初始化数据)
    bss = int(data[2])  # .bss 段 (未初始化数据)
    total = int(data[3])  # 总计
    
    # Flash 使用 = .text + .data
    flash_used = text + data_size
    # RAM 使用 = .data + .bss
    ram_used = data_size + bss
    
    return flash_used, ram_used

def format_size(size_bytes):
    """格式化字节大小为可读格式"""
    kb = size_bytes / 1024.0
    return f"{kb:.2f} KB"

def main():
    if len(sys.argv) != 2:
        print(f"用法: {sys.argv[0]} <elf文件路径>")
        sys.exit(1)
    
    elf_file = sys.argv[1]
    if not os.path.isfile(elf_file):
        print(f"错误: 文件 '{elf_file}' 不存在")
        sys.exit(1)
    
    output = get_size_output(elf_file)
    flash_used, ram_used = parse_size_output(output)
    
    flash_used_kb = flash_used / 1024.0
    ram_used_kb = ram_used / 1024.0
    
    flash_percent = (flash_used_kb / FLASH_SIZE) * 100
    ram_percent = (ram_used_kb / RAM_SIZE) * 100
    
    print("\n===== STM32 内存使用情况 =====")
    print(f"Flash 使用: {format_size(flash_used)} / {FLASH_SIZE} KB ({flash_percent:.2f}%)")
    print(f"RAM 使用: {format_size(ram_used)} / {RAM_SIZE} KB ({ram_percent:.2f}%)")
    print(f"Flash 剩余: {format_size(FLASH_SIZE*1024 - flash_used)} ({100-flash_percent:.2f}%)")
    print(f"RAM 剩余: {format_size(RAM_SIZE*1024 - ram_used)} ({100-ram_percent:.2f}%)")
    print("=============================\n")
    
    # 警告检查
    if flash_percent > 90:
        print("⚠️ 警告: Flash 使用率超过 90%")
    if ram_percent > 90:
        print("⚠️ 警告: RAM 使用率超过 90%")

if __name__ == "__main__":
    main() 