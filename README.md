## 借助ffmpeg + wasm实现网页截取视频帧功能

### 1. cfile/simple.c  
这个是纯C处理，把一个视频的第一帧保存为pcm图片，编译方法：
```bash
gcc simple.c -lavutil -lavformat -lavcodec `pkg-config --libs --cflags libavutil` `pkg-config --libs --cflags libavformat` `pkg-config --libs --cflags libavcodec` `pkg-config --libs --cflags libswscale` -o simple
```
使用方法，传递一个视频路径，就会在本地生成一个图片
```bash
./simple mountain.mp4
```
### 2. cfile/web.c proccess.c process.h
这个是网页版的核心C代码，使用以下命令编译成wasm：
```bash
emcc web.c process.c ../lib/libavformat.bc ../lib/libavcodec.bc ../lib/libswscale.bc ../lib/libswresample.bc ../lib/libavutil.bc -Os -s WASM=1 -o index.html -s EXTRA_EXPORTED_RUNTIME_METHODS='["ccall", "cwrap"]' -s ALLOW_MEMORY_GROWTH=1 -s TOTAL_MEMORY=167772160
```
注意上面的.bc文件顺序不能颠倒，被依赖的文件要往后放  
这个会生成index.wasm和index.js，在html引入
```html
<script src="index.js"></script>
```
它就会自动加载index.wasm文件

### 3. demo
这个是编译好的文件，需要使用http服务才能使用wasm：
```bash
# 在demo目录执行
# npm install -g http-server
http-server
```
然后访问http://localhost:8080/main.html
