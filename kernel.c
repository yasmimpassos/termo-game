#include "keyboard_map.h"
#include "lista_palavras.h"

#define LINES 25
#define COLUMNS_IN_LINE 80
#define BYTES_FOR_EACH_ELEMENT 2
#define SCREENSIZE (BYTES_FOR_EACH_ELEMENT * COLUMNS_IN_LINE * LINES)
#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_STATUS_PORT 0x64
#define IDT_SIZE 256
#define INTERRUPT_GATE 0x8e
#define KERNEL_CODE_SEGMENT_OFFSET 0x08
#define ENTER_KEY_CODE 0x1C

extern unsigned char keyboard_map[128];
extern void keyboard_handler(void);
extern char read_port(unsigned short port);
extern void write_port(unsigned short port, unsigned char data);
extern void load_idt(unsigned long *idt_ptr);

unsigned int current_loc = 0;
char *vidptr = (char*)0xb8000;

/* Configuração do jogo TERMO */
#define TAMANHO_PALAVRA 5
#define MAX_TENTATIVAS 6

char palavra_secreta[TAMANHO_PALAVRA+1];
char tentativa[TAMANHO_PALAVRA+1];
int tentativa_pos = 0;
int tentativas_done = 0;
int game_over = 0;

struct IDT_entry {
    unsigned short int offset_lowerbits;
    unsigned short int selector;
    unsigned char zero;
    unsigned char type_attr;
    unsigned short int offset_higherbits;
};

struct IDT_entry IDT[IDT_SIZE];

void printa(const char *str);
void printa_novaLinha(void);
void limpa_tela(void);
void verifica_palavra(void);

void idt_init(void) {
    unsigned long keyboard_address;
    unsigned long idt_address;
    unsigned long idt_ptr[2];

    keyboard_address = (unsigned long)keyboard_handler;
    IDT[0x21].offset_lowerbits = keyboard_address & 0xffff;
    IDT[0x21].selector = KERNEL_CODE_SEGMENT_OFFSET;
    IDT[0x21].zero = 0;
    IDT[0x21].type_attr = INTERRUPT_GATE;
    IDT[0x21].offset_higherbits = (keyboard_address & 0xffff0000) >> 16;

    write_port(0x20 , 0x11);
    write_port(0xA0 , 0x11);

    write_port(0x21 , 0x20);
    write_port(0xA1 , 0x28);

    write_port(0x21 , 0x00);
    write_port(0xA1 , 0x00);

    write_port(0x21 , 0x01);
    write_port(0xA1 , 0x01);

    write_port(0x21 , 0xff);
    write_port(0xA1 , 0xff);

    idt_address = (unsigned long)IDT ;
    idt_ptr[0] = (sizeof (struct IDT_entry) * IDT_SIZE) + ((idt_address & 0xffff) << 16);
    idt_ptr[1] = idt_address >> 16 ;

    load_idt(idt_ptr);
}

void kb_init(void) {
    write_port(0x21 , 0xFD);
}

void printa(const char *str) {
    unsigned int i = 0;
    while (str[i] != '\0') {
        vidptr[current_loc++] = str[i++];
        vidptr[current_loc++] = 0x07;
    }
}

void escolhe_palavra(int segundo) {
    int indice = segundo % TOTAL_PALAVRAS;
    for (int i = 0; i < TAMANHO_PALAVRA; i++) {
        palavra_secreta[i] = lista_palavras[indice][i];
    }
    palavra_secreta[TAMANHO_PALAVRA] = '\0';
}

void scroll_tela(void) {
    unsigned int line_size = BYTES_FOR_EACH_ELEMENT * COLUMNS_IN_LINE;

    // copia todas as linhas, menos a primeira
    for (unsigned int i = line_size; i < SCREENSIZE; i++) {
        vidptr[i - line_size] = vidptr[i];
    }

    // limpa a última linha
    for (unsigned int i = SCREENSIZE - line_size; i < SCREENSIZE; i += 2) {
        vidptr[i] = ' ';
        vidptr[i+1] = 0x07;
    }

    current_loc = SCREENSIZE - line_size; // cursor na nova última linha
}

void printa_novaLinha(void) {
    unsigned int line_size = BYTES_FOR_EACH_ELEMENT * COLUMNS_IN_LINE;
    current_loc = current_loc + (line_size - current_loc % (line_size));

    if (current_loc >= SCREENSIZE) {
        scroll_tela();
    }
}

void printa_comCor(const char *str, unsigned char cor) {
    unsigned int i = 0;
    while (str[i] != '\0') {
        vidptr[current_loc++] = str[i++];
        vidptr[current_loc++] = cor;
    }
}

void limpa_tela(void) {
    unsigned int i = 0;
    while (i < SCREENSIZE) {
        vidptr[i++] = ' ';
        vidptr[i++] = 0x07;
    }
    current_loc = 0;
}

/* Processa a palavra */
void verifica_entrada(void) {

    // Verifica se a palavra tem o tamanho correto
    if (tentativa_pos == TAMANHO_PALAVRA) {
        // Finaliza a string da tentativa
        tentativa[tentativa_pos] = '\0';

        // verifica a palavra digitada
        printa_novaLinha();
        verifica_palavra();
        printa_novaLinha();
    } else {
        // reseta a tentativa
        tentativa_pos = 0;

        // Mostra mensagem de erro
        printa_novaLinha();
        printa_comCor("Digite novamente, agora uma palavra com ", 0x0C);
        char str[2] = { '0' + TAMANHO_PALAVRA, '\0' };
        printa_comCor(str, 0x0C);
        printa_comCor(" letras!", 0x0C);
        printa_novaLinha();
    }
}

void keyboard_handler_main(void)
{
    unsigned char status;
    char keycode;

    write_port(0x20, 0x20);

    // Se o jogo acabou, não processa mais entradas
    if (game_over) return;

    status = read_port(KEYBOARD_STATUS_PORT);
    if (status & 0x01) {
        keycode = read_port(KEYBOARD_DATA_PORT);
        if(keycode < 0) return;

        if(keycode == ENTER_KEY_CODE) {
            verifica_entrada();
            return;
        }

        char c = keyboard_map[(unsigned char) keycode];
        if (c >= 'a' && c <= 'z') {
            if (tentativa_pos < TAMANHO_PALAVRA) {
                if (tentativa_pos == 0) {
                    printa_novaLinha();
                }
                tentativa[tentativa_pos++] = c;
                char str[2] = {c, '\0'};
                printa(str);
            }
        }
    }
}


/* Verifica se a palavra está certa */
void verifica_palavra() {
    int i, j;
    int iguais = 1;

    for (i = 0; i < TAMANHO_PALAVRA; i++) {

        // Converte a letra atual em string
        char str[2] = {tentativa[i], '\0'};

        // Para cada letra, verifica se está na posição correta
        if (tentativa[i] == palavra_secreta[i]) {
            printa_comCor(str, 0x0A);   // Printa em verde
        } else { 
            iguais = 0;
            int found = 0;

            // Passa por todas as letras da palavra secreta
            for (j = 0; j < TAMANHO_PALAVRA; j++) {
                if (tentativa[i] == palavra_secreta[j]) {
                    found = 1;
                    break;
                }
            }

            if (found) {
                printa_comCor(str, 0x0E); // Printa em amarelo
            } else {
                printa_comCor(str, 0x07); // Printa em cinza normal
            }
        }
    }

    printa_novaLinha();

    // Verifica se a tentativa está correta
    if (iguais) {
        printa_novaLinha();
        printa_comCor("Parabens! Voce acertou!", 0x0A);
        game_over = 1;
    } else {
        tentativas_done++;

        if (tentativas_done >= MAX_TENTATIVAS) {
            printa_novaLinha();
            printa_comCor("Game Over! A palavra era: ", 0x0C);
            printa_comCor(palavra_secreta, 0x0C);
            game_over = 1;
        }
    }

    tentativa_pos = 0;
}

// Função para ler um registrador do RTC
unsigned char read_rtc_register(int reg) {
    write_port(0x70, reg);
    return read_port(0x71);
}

int pega_segundo() {
    return read_rtc_register(0x00); // 0x00 = segundos
}


/* Da as instrucoes do jogo e o inicia */
void kmain(void)
{
    limpa_tela(); // Limpa a tela

    escolhe_palavra(pega_segundo());

    // Introdução do jogo
    printa_comCor("Vamos jogar termo?", 0x0D);
    printa_novaLinha();
    printa("Tente adivinhar a palavra de ");
    char str[2] = { '0' + TAMANHO_PALAVRA, '\0' };
    printa(str);
    printa(" letras. Voce tem ");
    char str2[2] = { '0' + MAX_TENTATIVAS, '\0' };
    printa(str2);
    printa(" tentativas.");

    printa_novaLinha();
    printa_novaLinha();

    // Instruções de cores
    printa("Letras na posicao correta aparecem em ");
    printa_comCor("verde.", 0x0A);
    printa_novaLinha();
    printa("Letras na palavra mas na posicao errada aparecem em ");
    printa_comCor("amarelo.", 0x0E);
    printa_novaLinha();
    printa("Letras nao na palavra aparecem em ");
    printa_comCor("cinza.", 0x07);
    printa_novaLinha();

    // Mensagem de boa sorte
    printa_comCor("Boa sorte!", 0x0D);
    printa_novaLinha();

    // Inicializa o IDT e o teclado
    idt_init();
    kb_init();

    while(1);
}
