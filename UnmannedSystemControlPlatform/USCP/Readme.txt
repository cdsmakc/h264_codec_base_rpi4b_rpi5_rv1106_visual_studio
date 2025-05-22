1. 需要OpenCV。线程VID需要使用OpenCV将YUV420图像转换为BGR24图像。
	1.1 OpenCV版本：4.5.4；
	1.2 OpenCV路径：D:\opencv；
	1.3 项目属性-->VC++目录-->库目录，添加路径“D:\opencv\build\x64\vc15\lib”；
	1.4 项目属性-->链接器-->输入-->附加依赖项，添加lib文件“opencv_world454d.lib”；
	1.5 项目属性-->C/C++-->常规-->附加包含目录，添加路径“D:\opencv\build\include\”；
	1.6 添加系统环境变量，将路径“D:\opencv\build\x64\vc15\bin”添加到系统环境变量（动态链接库）。
2. 需要ffmpeg。线程DEC使用ffmpeg对H264视频进行解码。
	2.1 ffmpeg版本：ffmpeg-7.0.2-essentials_build.zip；
	2.2 ffmpeg路径：D:\ffmpeg-7.0.2-full_build-shared；
	2.3 添加系统环境变量，将路径“D:\ffmpeg-7.0.2-full_build-shared\bin”添加到系统环境变量。
	2.4 项目属性-->VC++目录-->库目录，添加路径“D:\ffmpeg-7.0.2-full_build-shared\lib”；
	2.5 项目属性-->链接器-->输入-->附加依赖项，添加lib文件：“avcodec.lib”、“avdevice.lib”、“avfilter.lib”、“avformat.lib”、
	                                                 “avutil.lib”、“postproc.lib”、“swresample.lib”、“swscale.lib”
	2.6 项目属性-->C/C++-->常规-->附加包含目录，添加路径“D:\ffmpeg-7.0.2-full_build-shared\include”
3. 需要添加库“gdiplus.lib”。
	3.1 项目属性-->链接器-->输入-->附加依赖项，添加lib文件：“gdiplus.lib”。该库不需要指定路径。
4. 需要添加库“ws2_32.lib”。
	4.1 项目属性-->链接器-->输入-->附加依赖项，添加lib文件：“ws2_32.lib”。该库不需要指定路径。


	



