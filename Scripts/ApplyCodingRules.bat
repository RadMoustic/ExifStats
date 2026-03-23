@echo off

call Internal\Setup.bat 

python %ROOT%\Scripts\Internal\CodingRules.py %ROOT%/Inc %ROOT%/Src --vs-dir "%VS_PATH%" --exceptions "%ROOT%/CodingRulesExceptions.txt" --log-file "%GENERATED_PATH%/CodingRulesDebug.log" --replace-auto --spaces-to-tabs