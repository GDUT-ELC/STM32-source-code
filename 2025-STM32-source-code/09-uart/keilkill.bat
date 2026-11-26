@echo off
echo ------------------------------------------
echo         正在清理 Keil 编译垃圾文件...
echo ------------------------------------------

:: 1. 核心编译产物 (最占空间)
del *.o /s
del *.obj /s
del *.crf /s
del *.d /s
del *.axf /s
del *.htm /s
del *.dep /s
del *.map /s
del *.lst /s

:: 2. 现代 Keil V5 特有垃圾文件 (建议添加)
del *.uvguix.* /s
del *.build_log.htm /s
del *.scvd /s

:: 3. 临时文件和调试记录
del *.bak /s
del *.tmp /s
del JLinkLog.txt /s
del *.iex /s
del *.sct /s

:: 4. 兼容老版本 Keil 的清理项
del *.ddk /s
del *.edk /s
del *.lnp /s
del *.mpf /s
del *.mpj /s
del *.omf /s
del *.plg /s
del *.rpt /s
del *.tra /s
del *.__i /s

:: 5. 下面这行保留注释，不要删除 J-Link 设置
:: del *.opt /s

echo ------------------------------------------
echo               清理完成 !
echo ------------------------------------------
pause