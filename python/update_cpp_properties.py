import json
import os

def parse_makefile(makefile_path):
    with open(makefile_path, 'r') as f:
        content = f.read()

    # 解析 C_SOURCES
    c_sources_start = content.find('C_SOURCES =') + len('C_SOURCES =')
    c_sources_end = content.find('\n\n', c_sources_start)
    c_sources = content[c_sources_start:c_sources_end].strip().split(' \\\n')
    c_sources = [s.strip() for s in c_sources if s.strip()]
    
    # 解析 C_INCLUDES
    c_includes_start = content.find('C_INCLUDES =') + len('C_INCLUDES =')
    c_includes_end = content.find('\n\n', c_includes_start)
    c_includes = content[c_includes_start:c_includes_end].strip().split(' \\\n')
    c_includes = [s.strip().replace('-I', '').replace('\\\n', '') for s in c_includes if s.strip()]
    
    # 解析 AS_INCLUDES
    as_includes_start = content.find('AS_INCLUDES =') + len('AS_INCLUDES =')
    as_includes_end = content.find('\n\n', as_includes_start)
    as_includes = content[as_includes_start:as_includes_end].strip().split(' \\\n')
    as_includes = [s.strip().replace('-I', '').replace('\\\n', '') for s in as_includes if s.strip()]
    
    # 解析 C_DEFS
    c_defs_start = content.find('C_DEFS =') + len('C_DEFS =')
    c_defs_end = content.find('\n\n', c_defs_start)
    c_defs = content[c_defs_start:c_defs_end].strip().split(' \\\n')
    c_defs = [s.strip().replace('-D', '').replace('\\\n', '') for s in c_defs if s.strip()]
    
    return c_sources, c_includes, as_includes, c_defs

def get_source_dirs(c_sources):
    # 获取源文件所在的目录
    dirs = set()
    for source in c_sources:
        dir_path = os.path.dirname(source)
        if dir_path:
            dirs.add(dir_path)
    return list(dirs)

def update_cpp_properties(cpp_properties_path, include_paths, defines):
    with open(cpp_properties_path, 'r') as f:
        data = json.load(f)
    
    # 准备包含路径，添加 /** 后缀，移除重复和清理 \\\n
    formatted_paths = ["${workspaceFolder}/**"]
    seen_paths = set()
    seen_paths.add("${workspaceFolder}/**")
    
    for path in include_paths:
        if path and not path.startswith('-'):  # 忽略以 - 开头的路径
            clean_path = path.replace('\\\n', '').strip()
            formatted_path = f"{clean_path}/**"
            if formatted_path not in seen_paths:
                formatted_paths.append(formatted_path)
                seen_paths.add(formatted_path)
    
    # 更新配置
    if 'configurations' in data:
        for config in data['configurations']:
            config['includePath'] = formatted_paths
            config['defines'] = [d.replace('\\\n', '') for d in defines]

    # 写回文件
    with open(cpp_properties_path, 'w') as f:
        json.dump(data, f, indent=4)

def main():
    # 解析 Makefile
    c_sources, c_includes, as_includes, c_defs = parse_makefile('Makefile')
    
    # 合并所有包含路径
    include_paths = set()
    include_paths.update(c_includes)
    include_paths.update(as_includes)
    include_paths.update(get_source_dirs(c_sources))
    
    # 移除重复项并排序
    include_paths = sorted(list(include_paths))
    
    # 更新 c_cpp_properties.json
    update_cpp_properties('.vscode/c_cpp_properties.json', include_paths, c_defs)
    
    print("配置文件已更新完成！")

if __name__ == "__main__":
    main() 