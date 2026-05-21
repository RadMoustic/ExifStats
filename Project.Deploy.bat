if "%IMAGETAGGER_ENABLE%" == "true" (
	xcopy /D /Y "%ROOT%\Externals\onnxruntime\bin\*.*" "%OUTPUT_BIN_DIR%\"
)