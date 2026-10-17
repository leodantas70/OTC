# Assistente nativo - Old FasterOT 7.72

Esta versao do cliente inclui um assistente interno, sem leitura/injecao de memoria e sem simulacao externa de teclado.

## Como configurar

1. Execute `Tibia-Assistant.exe` na pasta principal do Old FasterOT.
2. Abra `Options` e clique em `Assistant`.
3. Marque `Ativar automacoes` para habilitar cura, haste, mana e a tecla de runa.
4. `Full light` pode ser ligado ou desligado separadamente.
5. Preencha os IDs e subtipos usados pelo servidor e clique em `Ok`.

As configuracoes sao salvas em `assistant.cfg`, na mesma pasta de preferencias usada pelo cliente.

## Funcoes

- Cura abaixo da porcentagem configurada por magia ou runa.
- Haste automatica quando o icone de haste nao estiver ativo.
- Mana fluid abaixo da porcentagem configurada.
- Runa de ataque no alvo de batalha atual por uma tecla configuravel (F8 por padrao).
- Energy ring de emergencia, com troca rapida e retirada automatica acima da porcentagem configurada.
- Full light com restauracao do modo de luz anterior ao desligar ou fechar o cliente.
- `EXP/hour` visivel no painel Skills; o botao Analytics tambem abre esse painel quando estiver disponivel no layout.
- `Exiva Nome` no menu de contexto ao clicar com o botao direito em um jogador da VIP.
- A mao do inventario alterna o Secure Mode (PvP) usando o protocolo compativel com 7.72.

## Observacoes

- Todos os recursos iniciam desativados.
- IDs e subtipos ficam em zero, exceto o subtipo padrao do mana fluid (`7`). Confirme esses valores no seu servidor antes de ativar.
- As acoes automaticas usam prioridade `cura -> mana -> haste` e intervalo minimo de 650 ms.
- O perfil padrao do projeto e o Old FasterOT 7.72 encontrado no pacote desta pasta. O perfil 8.60 original do repositorio foi mantido como alternativa em `data/features.lua`.
- Use apenas onde as regras do servidor permitem automacao.

## Compilacao no Windows

O toolchain Microsoft C++ esta instalado em `C:\BuildTools2022`. Compile `src/visual-studio/visual-studio.sln` em `Release|x86`, pois o cliente e suas DLLs sao de 32 bits. O executavel e gerado em `data/Tibia.exe`; os arquivos 7.72 do pacote ja foram colocados em `data/Assets`.

O arquivo `features-assistant.lua` e exclusivo desta compilacao. O `features.lua` e o `Tibia.exe` originais nao sao alterados.
