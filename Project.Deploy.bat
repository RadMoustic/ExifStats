if "%IMAGETAGGER_ENABLE%" == "true" (
	xcopy /D /Y "%ROOT%\Externals\onnxruntime\bin\*.*" "%OUTPUT_BIN_DIR%\"
	xcopy /D /Y "%ROOT%\rc\ImageTaggers\*.*" "%OUTPUT_BIN_DIR%\ImageTaggers\"
	xcopy /D /Y "%ROOT%\rc\Tokenizer\*.*" "%OUTPUT_BIN_DIR%\Tokenizer\"
)