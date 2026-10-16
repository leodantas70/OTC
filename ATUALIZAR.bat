@echo off
setlocal EnableExtensions

if /i "%~1"=="--executar-temp" goto :atualizar

rem Executa por uma copia temporaria para permitir atualizar este proprio arquivo.
for %%I in ("%~dp0.") do set "OTC_ALVO=%%~fI"
set "OTC_RUNNER=%TEMP%\otc-atualizar-%RANDOM%-%RANDOM%.bat"
copy /y "%~f0" "%OTC_RUNNER%" >nul
if errorlevel 1 (
  echo Nao foi possivel preparar o atualizador.
  pause
  exit /b 1
)

call "%OTC_RUNNER%" --executar-temp "%OTC_ALVO%"
set "OTC_RESULTADO=%ERRORLEVEL%"
del /q "%OTC_RUNNER%" >nul 2>nul
exit /b %OTC_RESULTADO%

:atualizar
title Atualizar OTC - Tibia Assistant
for %%I in ("%~2\.") do set "OTC_ALVO=%%~fI"
cd /d "%OTC_ALVO%"

echo ========================================
echo       ATUALIZADOR OTC ASSISTANT
echo ========================================
echo.
echo Baixando a versao mais recente do GitHub...

where powershell.exe >nul 2>nul
if errorlevel 1 (
  echo.
  echo ERRO: O PowerShell do Windows nao foi encontrado.
  goto :erro
)

rem Fecha apenas o cliente modificado para liberar o executavel durante a copia.
taskkill /F /IM Tibia-Assistant.exe >nul 2>nul

set "OTC_ZIP=%TEMP%\otc-%RANDOM%-%RANDOM%.zip"
set "OTC_TMP=%TEMP%\otc-%RANDOM%-%RANDOM%"
set "OTC_FONTE_TXT=%TEMP%\otc-fonte-%RANDOM%-%RANDOM%.txt"

powershell.exe -NoProfile -ExecutionPolicy Bypass -Command ^
  "$ErrorActionPreference='Stop'; $ProgressPreference='SilentlyContinue';" ^
  "Invoke-WebRequest -UseBasicParsing 'https://github.com/leodantas70/OTC/archive/refs/heads/main.zip' -OutFile $env:OTC_ZIP;" ^
  "Expand-Archive -LiteralPath $env:OTC_ZIP -DestinationPath $env:OTC_TMP -Force;" ^
  "$src=(Get-ChildItem -LiteralPath $env:OTC_TMP -Directory | Select-Object -First 1).FullName;" ^
  "if(-not $src){throw 'Conteudo da atualizacao nao encontrado'};" ^
  "Set-Content -LiteralPath $env:OTC_FONTE_TXT -Value $src -Encoding ASCII"
if errorlevel 1 goto :erro

set /p "OTC_FONTE="<"%OTC_FONTE_TXT%"
if not defined OTC_FONTE goto :erro
if not exist "%OTC_FONTE%\Tibia-Assistant.exe" goto :erro
if not exist "%OTC_FONTE%\Assets\Tibia.dat" goto :erro

echo.
echo Atualizando executaveis, configuracoes e Assets...
for %%F in (Tibia-Assistant.exe Tibia.exe zlib1.dll features.lua features-assistant.lua ASSISTANT.md ATUALIZAR.bat SHA256SUMS.txt) do (
  if exist "%OTC_FONTE%\%%F" copy /y "%OTC_FONTE%\%%F" "%OTC_ALVO%\%%F" >nul
  if errorlevel 1 goto :erro
)

if not exist "%OTC_ALVO%\Assets" mkdir "%OTC_ALVO%\Assets"
robocopy "%OTC_FONTE%\Assets" "%OTC_ALVO%\Assets" /E /COPY:DAT /R:2 /W:1 /NFL /NDL /NJH /NJS /NP
if errorlevel 8 goto :erro

:limpar_sucesso
if exist "%OTC_TMP%" rmdir /s /q "%OTC_TMP%"
if exist "%OTC_ZIP%" del /q "%OTC_ZIP%"
if exist "%OTC_FONTE_TXT%" del /q "%OTC_FONTE_TXT%"
echo.
echo ========================================
echo   ATUALIZACAO CONCLUIDA COM SUCESSO!
echo ========================================
echo Suas contas salvas, automap e logs locais foram preservados.
echo Abra o Tibia-Assistant.exe para jogar.
pause
exit /b 0

:erro
if defined OTC_TMP if exist "%OTC_TMP%" rmdir /s /q "%OTC_TMP%"
if defined OTC_ZIP if exist "%OTC_ZIP%" del /q "%OTC_ZIP%"
if defined OTC_FONTE_TXT if exist "%OTC_FONTE_TXT%" del /q "%OTC_FONTE_TXT%"
echo.
echo ========================================
echo        A ATUALIZACAO FALHOU
echo ========================================
echo Confira sua internet, o acesso ao GitHub e tente novamente.
pause
exit /b 1
