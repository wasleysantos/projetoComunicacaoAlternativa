<div align="center" >
  <img align="center" alt="Rafa-Python" height="200" width="350" src="https://ifce.edu.br/noticias/ifce-integra-capacitacao-nacional-em-sistemas-embarcados/captura-de-tela-2024-07-08-141318-1.jpg/@@images/54478835-b15d-4db9-84b1-7b0da1d40311.jpeg">
</div>

# TalkGo

O objetivo deste projeto é facilitar a comunicação de pessoas com deficiência motora e/ou na fala, através de um sistema acessível que permite que o 
usuário escolha sua fala utilizando um joystick. A mensagem gerada é então apresentada de forma compreensível para quem a recebe através do LCD. Muitas vezes, a comunicação dessas pessoas é dificultada pelas condições de saúde e pela falta de soluções acessíveis. Com isso, meu objetivo é oferecer uma alternativa eficaz, especialmente considerando que os 
sistemas existentes são geralmente caros e de difícil acesso. Quero criar uma ferramenta inclusiva e de fácil utilização para melhorar a interação e a qualidade de vida dessas pessoas.

## 🚀Começando

Essas instruções permitirão que você obtenha uma cópia do projeto em operação na sua máquina local para fins de desenvolvimento e teste.

### 📋 Pré-requisitos

Antes de iniciar, certifique-se de que possui os seguintes itens:

BitDogLab ou Simulador Wokwi

Display LCD 16x2 (conectado via I2C nos pinos 4 e 5)

Módulo Joystick

Ambiente de desenvolvimento C/C++ configurado com o SDK do Raspberry Pi Pico

Ferramentas de compilação:

CMake

GNU Arm Embedded Toolchain

Python 3 (para conversão de UF2)

OpenOCD (para upload do código)

### 🔧 Instalação

Siga os passos abaixo para configurar o ambiente:

Clone o repositório

git clone https://github.com/usuario/TalkGo.git
cd TalkGo

Configure o SDK do Raspberry Pi Pico (caso ainda não tenha instalado)

export PICO_SDK_PATH=/caminho/para/o/sdk/pico-sdk

Crie o diretório de build e compile o projeto

mkdir build && cd build
cmake ..
make

Carregue o firmware no Raspberry Pi Pico

Conecte o Raspberry Pi Pico ao seu computador enquanto pressiona o botão BOOTSEL

Arraste e solte o arquivo .uf2 gerado na unidade montada

## ⚙️ Executando os testes

Explicar como executar os testes automatizados para este sistema.

### 🔩 Analise os testes de ponta a ponta

Explique que eles verificam esses testes e porquê.

```
Dar exemplos
```

### ⌨️ E testes de estilo de codificação

Explique que eles verificam esses testes e porquê.

```
Dar exemplos
```

## 📦 Implantação

Adicione notas adicionais sobre como implantar isso em um sistema ativo

## 🛠️ Construído com

Raspberry Pi Pico SDK - Desenvolvimento em C

CMake - Sistema de build

GNU Arm Toolchain - Compilador para ARM

## 🖇️ Colaborando

Por favor, leia o [COLABORACAO.md](https://gist.github.com/usuario/linkParaInfoSobreContribuicoes) para obter detalhes sobre o nosso código de conduta e o processo para nos enviar pedidos de solicitação.

## 📌 Versão

Nós usamos [SemVer](http://semver.org/) para controle de versão. Para as versões disponíveis, observe as [tags neste repositório](https://github.com/suas/tags/do/projeto). 

## ✒️ Autores

[Seu Nome] - Desenvolvimento e Implementação

Colaboradores - Contribuições adicionais

## 📄 Licença

Este projeto está sob a licença (sua licença) - veja o arquivo [LICENSE.md](https://github.com/usuario/projeto/licenca) para detalhes.

## 🎁 Expressões de gratidão

A todos que apoiaram o desenvolvimento do TalkGo! 🚀

Se este projeto ajudou você, considere compartilhar ou contribuir! 📢
