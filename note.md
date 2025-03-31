数量分别为：
NameUnmatch TypeUnmatch MemberSizeUnmatch
0 49 28

18.json是aarch64，扫出来的东西包括typedef在内一共有2077
19.json是x86，扫出来的东西包括typedef在内一共有2015

======================================================

glibc 2.40
编译过程中的解决：

https://dbl2017.github.io/post/%E5%BC%80%E6%BA%90%E4%B8%89%E6%96%B9/ubuntu24.04%E7%8E%AF%E5%A2%83%E7%BC%96%E8%AF%91glibc-2.40%E9%94%99%E8%AF%AF%E8%AE%B0%E5%BD%95/

Problem 1:
wctomb.c:57:1: error: ‘artificial’ attribute ignored [-Werror=attributes]
   57 | libc_hidden_def (wctomb)
solution: ../configure CFLAGS="-U_FORTIFY_SOURCE"

Problem 2:
In file included from <command-line>:
./../include/libc-symbols.h:75:3: error: #error "glibc cannot be compiled without optimization"
   75 | # error "glibc cannot be compiled without optimization"
      |   ^~~~~
In file included from <command-line>:
./../include/libc-symbols.h:75:3: error: #error "glibc cannot be compiled without optimization"
   75 | # error "glibc cannot be compiled without optimization"
      |   ^~~~~
In file included from <command-line>:
./../include/libc-symbols.h:75:3: error: #error "glibc cannot be compiled without optimization"
   75 | # error "glibc cannot be compiled without optimization"
      |   ^~~~~
In file included from <command-line>:
./../include/libc-symbols.h:75:3: error: #error "glibc cannot be compiled without optimization"
   75 | # error "glibc cannot be compiled without optimization"
      |   ^~~~~
In file included from <command-line>:
./../include/libc-symbols.h:75:3: error: #error "glibc cannot be compiled without optimization"
   75 | # error "glibc cannot be compiled without optimization"
      |   ^~~~~
In file included from <command-line>:
./../include/libc-symbols.h:75:3: error: #error "glibc cannot be compiled without optimization"
   75 | # error "glibc cannot be compiled without optimization"
      |   ^~~~~
make[2]: *** [../o-iterator.mk:9: /home/blduan/projects/open_source/glibc-2.40/build/stdlib/inlines.os] Error 1
make[2]: *** Waiting for unfinished jobs....
make[2]: *** [../o-iterator.mk:9: /home/blduan/projects/open_source/glibc-2.40/build/stdlib/divrem.os] Error 1
make[2]: *** [../o-iterator.mk:9: /home/blduan/projects/open_source/glibc-2.40/build/stdlib/mul.os] Error 1
make[2]: *** [../o-iterator.mk:9: /home/blduan/projects/open_source/glibc-2.40/build/stdlib/divmod_1.os] Error 1
make[2]: *** [../o-iterator.mk:9: /home/blduan/projects/open_source/glibc-2.40/build/stdlib/mod_1.os] Error 1
make[2]: *** [../o-iterator.mk:9: /home/blduan/projects/open_source/glibc-2.40/build/stdlib/wctomb.os] Error 1
make[2]: Leaving directory '/home/blduan/projects/open_source/glibc-2.40/stdlib'
make[1]: *** [Makefile:484: stdlib/subdir_lib] Error 2
make[1]: Leaving directory '/home/blduan/projects/open_source/glibc-2.40'
make: *** [Makefile:9: all] Error 2

solution ../configure CFLAGS="-U_FORTIFY_SOURCE -O2"

glibc x86编译配置
../configure CFLAGS="-U_FORTIFY_SOURCE -O2" --prefix=/opt/glibc-x86
bear -- make

gilbc aarch64编译配置
../configure CFLAGS="-U_FORTIFY_SOURCE -O2" --prefix=/opt/glibc-aarch64 --host=aarch64-none-linux-gnu
bear -- make

=============================================================

data_encode.h data_encode.cpp : json文件的encode和decode
FindClassDecls.cpp : 提取定义，生成json
json_cmp.cpp ： json文件的对比

clangtool要和别的clangtool放在一起，比如/home/ubuntu/install/bin,这个includePath有关



===========================================================

1/30版本后的主要改动：
1 修改比较标准，有能够匹配上的则被认为是匹配上了
2 相同名称不同定义的数据结构

===========================================================

stat结构体的包含路径
sys/stat.h -> io/sys/stat.h -> bits/stat.h -> bits/struct_stat.h


===========================================================

之前统计出来的数据:
aarch64 共2271	独有382
x86 共2125	独有231
Sum NameUnmatch TypeUnmatch MemberSizeUnmatch
Sum 0 50 39
数据貌似有点问题，我再看看哪里出了问题

现在统计出来的数据：
AArch64 共1974个
x86 共2115个

==========================================================

获取大小的方案
ASTContext中获取ASTRecordLayout

// In ASTContext
public: uint64_t getTypeSize(QualType T) const

===========================================================

分类有重叠，将mismatch作为一种label，给差异打标签


_Float128:
gcc testfloat128.c -lquadmath -o float
clang并没有支持_Float128，因此这里直接跳过判断