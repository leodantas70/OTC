# OTC - Tibia Assistant

Cliente customizado baseado no projeto open source The Forgotten Client, preparado para o protocolo do Old FasterOT e acompanhado do painel Assistant.

## Como usar

1. Baixe o ZIP da versao mais recente do repositorio.
2. Extraia todos os arquivos para uma pasta.
3. Execute `Tibia-Assistant.exe`.

O `Tibia.exe` original tambem esta incluido e nao e modificado pelo Assistant.

## Atualizar em outras maquinas

Execute `ATUALIZAR.bat` dentro da pasta instalada. Ele baixa a branch `main`, encerra somente o `Tibia-Assistant.exe` caso esteja aberto e atualiza:

- executaveis e DLL necessaria;
- arquivos `features*.lua`;
- pasta `Assets`;
- o proprio atualizador.

O atualizador preserva `saved_accounts.tfc`, `Automap`, `logs` e outros dados locais.

## Codigo-fonte

O codigo alterado esta em `source-client`. Para compilar no Windows, abra a solucao em `source-client/src/visual-studio/visual-studio.sln` com suporte C++ x86 e compile a configuracao `Release|x86`.

## Creditos e licenca

Baseado em [The Forgotten Client](https://github.com/Mateuzkl/The-Forgotten-Client). Consulte `source-client/COPYING.txt` e os avisos de licenca do projeto original.
