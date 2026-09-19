#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""统计项目代码总行数（文件行数累加），分为"含库文件"与"不含库文件"两类。

用法: python count_lines.py [项目根目录，默认本脚本所在目录]
"""
import os
import sys

EXTS = {'.h', '.hpp', '.hh', '.inl', '.c', '.cpp', '.cc', '.cxx', '.lua', '.rc'}

# 编译产物 / 缓存 / 打包目录，不计入任何统计
EXCLUDE_DIRS = {'x64', 'Debug', 'Release', '.codegraph', '.reasonix',
                'packages', 'script-docs', 'GC安装文件', 'GameClciker3.0'}

# 第三方库目录：计入"含库文件"，不计入"不含库文件"
LIB_DIRS = {'ImGui', 'OpenAL', 'stb', 'zlib', 'libwebp', 'LuaJIT', 'json', 'whale', 'GLEW'}


def count_lines_in_file(path):
    try:
        with open(path, 'rb') as f:
            data = f.read()
    except OSError:
        return 0
    if not data:
        return 0
    return data.count(b'\n') + (0 if data.endswith(b'\n') else 1)


def main():
    root = sys.argv[1] if len(sys.argv) > 1 else os.path.dirname(os.path.abspath(__file__))
    root = os.path.abspath(root)

    total_with_lib = 0
    total_no_lib = 0
    own_files = []
    lib_files = []

    for dirpath, dirnames, filenames in os.walk(root):
        dirnames[:] = [d for d in dirnames if d not in EXCLUDE_DIRS]
        rel = os.path.relpath(dirpath, root)
        first = rel.split(os.sep)[0]
        in_lib = first in LIB_DIRS

        for fn in filenames:
            if os.path.splitext(fn)[1].lower() not in EXTS:
                continue
            p = os.path.join(dirpath, fn)
            n = count_lines_in_file(p)
            total_with_lib += n
            if in_lib:
                lib_files.append((os.path.relpath(p, root), n))
            else:
                own_files.append((os.path.relpath(p, root), n))
                total_no_lib += n

    print('项目根目录 : %s' % root)
    print('统计扩展名 : %s' % ', '.join(sorted(EXTS)))
    print('排除目录   : %s' % ', '.join(sorted(EXCLUDE_DIRS)))
    print('第三方库目录: %s' % ', '.join(sorted(LIB_DIRS)))
    print('-' * 64)
    print('总行数（含库文件）   : %d' % total_with_lib)
    print('总行数（不含库文件） : %d' % total_no_lib)
    print('库文件行数           : %d' % (total_with_lib - total_no_lib))
    print('-' * 64)

    own_files.sort(key=lambda x: -x[1])
    print('不含库文件 > 前20大:')
    for path, n in own_files[:20]:
        print('   %8d  %s' % (n, path))
    if len(own_files) > 20:
        print('   ... 其余 %d 个文件' % (len(own_files) - 20))
    print('共 %d 个文件（不含库）' % len(own_files))

    lib_files.sort(key=lambda x: -x[1])
    print('含库文件 > 前10大:')
    for path, n in lib_files[:10]:
        print('   %8d  %s' % (n, path))
    print('共 %d 个库文件' % len(lib_files))


if __name__ == '__main__':
    main()
