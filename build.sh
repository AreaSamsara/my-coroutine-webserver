# 检查 build 文件夹是否存在，如果不存在则创建
if [ ! -d "build" ]; then
    mkdir build
fi

# 进入 build 文件夹
cd build

# 使用 cmake 生成 Makefile
cmake ..

# 使用 make 编译项目
make

# 返回到上一级目录
cd ..