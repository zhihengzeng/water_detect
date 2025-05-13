def convert_8x16_font():
    """转换8x16字库为结构体形式"""
    ascii_chars = ' !"#$%&\'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~'
    result = []
    result.append("static const FONT8x16 Font8x16[] = {")
    
    with open('../App/oledfont.h', 'r') as f:
        lines = f.readlines()
        
    i = 0
    data_block = []
    for line in lines:
        if 'F8X16[]' in line:
            start_found = True
            continue
            
        if '0x' in line and '//' in line:
            data = line.split('//')[0].strip().rstrip(',')
            comment = line.split('//')[-1].strip()
            if len(data_block) == 0:
                char_index = int(comment.split()[-1])
                if char_index < len(ascii_chars):
                    current_char = ascii_chars[char_index]
                    data_block.append(data)
            else:
                data_block.append(data)
                
            if len(data_block) == 16:
                result.append(f"    {{'{current_char}', {{{', '.join(data_block)}}}}},  // {comment}")
                data_block = []
                
    result.append("};")
    return result

def convert_6x8_font():
    """转换6x8字库为结构体形式"""
    ascii_chars = ' !"#$%&\'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~'
    result = []
    result.append("static const FONT6x8 Font6x8[] = {")
    
    with open('../App/oledfont.h', 'r') as f:
        lines = f.readlines()
        
    for line in lines:
        if '0x' in line and '//' in line and len(line.split(',')) == 6:
            data = line.split('//')[0].strip().rstrip(',')
            comment = line.split('//')[-1].strip()
            index = ascii_chars.find(comment.strip())
            if index >= 0:
                result.append(f"    {{'{ascii_chars[index]}', {{{data}}}}},  // {comment}")
                
    result.append("};")
    return result

def convert_chinese_font():
    """转换中文字库为结构体形式"""
    result = []
    result.append("static const FONT16x16 Hzk[] = {")
    
    with open('../App/oledfont.h', 'r') as f:
        lines = f.readlines()
        
    i = 0
    data_block = []
    for line in lines:
        if '/*"' in line:
            name = line.split('/*"')[1].split('"')[0]
            data = line.split('/*')[0].strip().rstrip(',')
            if len(data_block) == 0:
                data_block.append(data)
            else:
                data_block.append(data)
                result.append(f"    {{{{{', '.join(data_block)}}}, \"{name}\"}},")
                data_block = []
                
    result.append("};")
    return result

def main():
    # 生成新的字库文件内容
    header = """#ifndef __OLEDFONT_H
#define __OLEDFONT_H

// 字符字模结构体 8x16
typedef struct {
    char ascii;           // ASCII字符
    uint8_t data[16];    // 16字节的字模数据(8x16字体)
} FONT8x16;

// 字符字模结构体 6x8
typedef struct {
    char ascii;          // ASCII字符
    uint8_t data[6];    // 6字节的字模数据(6x8字体)
} FONT6x8;

// 中文字模结构体 16x16
typedef struct {
    uint8_t data[32];    // 32字节的字模数据(16x16字体)
    char name[8];        // 汉字名称
} FONT16x16;

"""
    
    # 转换并写入新文件
    with open('../App/oledfont_new.h', 'w') as f:
        f.write(header)
        f.write('\n'.join(convert_6x8_font()))
        f.write('\n\n')
        f.write('\n'.join(convert_8x16_font()))
        f.write('\n\n')
        f.write('\n'.join(convert_chinese_font()))
        f.write('\n\n#endif\n')

if __name__ == '__main__':
    main()