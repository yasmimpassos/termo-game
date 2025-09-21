# Micro Kernel com Jogo TERMO  

## Demonstração em Vídeo  
[Clique aqui para assistir](./explicacao/demo.webm)  


## Sobre o Projeto  
Este projeto é a implementação de um **micro kernel** simples em Assembly e C, capaz de:  
- Inicializar via **bootloader**;  
- Escrever caracteres na tela usando **memória de vídeo**;  
- Configurar a **IDT (Interrupt Descriptor Table)** e lidar com **interrupções do teclado**;  
- Executar uma aplicação/jogo interativo no kernel.  

O jogo escolhido foi uma versão simplificada do **TERMO** (inspirado em Wordle), onde o jogador deve adivinhar uma palavra secreta de 5 letras.  


## O Jogo TERMO no Kernel  
- São **60 palavras possíveis**, todas com 5 letras, escolhidas de acordo com o **segundo em que o kernel foi inicializado**.  
- O jogador tem **6 tentativas** para adivinhar a palavra correta.  
- As letras são exibidas com **cores diferentes**:  
  - 🟩 Verde → letra na posição correta;  
  - 🟨 Amarelo → letra existe, mas está na posição errada;  
  - ⬜ Cinza → letra não existe na palavra.  

### Regras adicionais  
- Só é permitido digitar **5 letras por tentativa**;  
- Se digitar menos letras, a tentativa não conta e o jogo pede para refazer;  
- Não é possível apagar letras já digitadas;  
- Se a palavra secreta contém apenas **uma letra repetida** mas o jogador digita duas vezes, ambas podem aparecer em amarelo (já que a verificação apenas checa se a letra existe, e não a quantidade exata).  


## Estrutura do Projeto  
O projeto é composto pelos seguintes arquivos principais:  

- **kernel.asm** → Bootloader em Assembly, inicializa o kernel e define funções auxiliares;  
- **kernel.c** → Lógica do kernel e do jogo TERMO (impressão, tratamento de teclado, jogo);  
- **keyboard_map.h** → Mapa de teclas para conversão de scancodes;  
- **lista_palavras.h** → Lista de palavras possíveis para o jogo;  
- **link.ld** → Script de linkagem para gerar o binário final.  


## Referências utilizadas  
Este projeto foi construído a partir dos seguintes tutoriais como base:  

- Criar um kernel:  
  [Kernels 101 - Let's write a Kernel](https://arjunsreedharan.org/post/82710718100/kernels-101-lets-write-a-kernel)  

- Input do teclado:  
  [Kernels 201 - Let's write a Kernel with keyboard](https://arjunsreedharan.org/post/99370248137/kernels-201-lets-write-a-kernel-with-keyboard)  

A partir deles, foi incorporada a lógica completa do jogo **TERMO**.  


## Como Rodar  

Clone o repositório e rode com o QEMU:  

```bash
qemu-system-i386 -kernel kernel
```

## Como Modificar  

Se você alterar os arquivos, precisa recompilar os objetos e **sempre rodar o comando final de linkagem**:  

### Caso altere apenas o Assembly (`kernel.asm`):  
```bash
nasm -f elf kernel.asm -o kasm.o
ld -m elf_i386 -T link.ld -o kernel kasm.o kc.o
```

### Caso altere apenas o C (`kernel.c`):  
```bash
gcc -m32 -ffreestanding -fno-stack-protector -c kernel.c -o kc.o
ld -m elf_i386 -T link.ld -o kernel kasm.o kc.o
```

### Caso altere ambos:  
```bash
nasm -f elf kernel.asm -o kasm.o
gcc -m32 -ffreestanding -fno-stack-protector -c kernel.c -o kc.o
ld -m elf_i386 -T link.ld -o kernel kasm.o kc.o
```

Depois, execute novamente com o QEMU:  
```bash
qemu-system-i386 -kernel kernel
```