#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/usb.h>

// Envia um comando via USB, espera e retorna a resposta do dispositivo (convertido para int)
// Exemplo de Comando:  SET_LED 80
// Exemplo de Resposta: RES SET_LED 1
static int usb_send_cmd2(char *cmd, int param) {
    int recv_size = 0;                      // Quantidade de caracteres no recv_line
    int ret, actual_size, i;
    int retries = 10;                       // Tenta algumas vezes receber uma resposta da USB. Depois desiste.
    char resp_expected[MAX_RECV_LINE];      // Resposta esperada do comando
    char *resp_pos;                         // Posição na linha lida que contém o número retornado pelo dispositivo
    long resp_number = -1;                  // Número retornado pelo dispositivo (e.g., valor do led, valor do ldr)
    int ignore;                             // Variável declarada para armazenar o retorno de kstrtol

    printk(KERN_INFO "SmartLamp: Enviando comando: %s\n", cmd);

    if (param >= 0) sprintf(usb_out_buffer, "%s %d\n", cmd, param); // Se param >=0, o comando possui um parâmetro (int)
    else sprintf(usb_out_buffer, "%s\n", cmd);                      // Caso contrário, é só o comando mesmo

    // Envia o comando (usb_out_buffer) para a USB
    ret = usb_bulk_msg(smartlamp_device, usb_sndbulkpipe(smartlamp_device, usb_out), usb_out_buffer, strlen(usb_out_buffer), &actual_size, 1000*HZ);
    if (ret) {
        printk(KERN_ERR "SmartLamp: Erro de codigo %d ao enviar comando!\n", ret);
        return -1;
    }

    sprintf(resp_expected, "RES %s", cmd);  // Resposta esperada. Ficará lendo linhas até receber essa resposta.

    // Espera pela resposta correta do dispositivo (desiste depois de várias tentativas)
    while (retries > 0) {
        // Lê dados da USB
        ret = usb_bulk_msg(smartlamp_device, usb_rcvbulkpipe(smartlamp_device, usb_in), usb_in_buffer, min(usb_max_size, MAX_RECV_LINE), &actual_size, HZ*1000);
        if (ret) {
            printk(KERN_ERR "SmartLamp: Erro ao ler dados da USB (tentativa %d). Codigo: %d\n", ret, retries--);
            continue;
        }

        // Para cada caractere recebido ...
        for (i = 0; i < actual_size; i++) {
            // HASHDJKHDASKJDRES GET_LED 100asdjahdjakdhaskjdh\n
            if (usb_in_buffer[i] == '\n') {  // Temos uma linha completa
                recv_line[recv_size] = '\0';
                printk(KERN_INFO "SmartLamp: Recebido uma linha: '%s'\n", recv_line);
 
                // Verifica se o início da linha recebida é igual à resposta esperada do comando enviado
                if (!strncmp(recv_line, resp_expected, strlen(resp_expected))) {
                    printk(KERN_INFO "SmartLamp: Linha eh resposta para %s! Processando ...\n", cmd);

                    // Acessa a parte da resposta que contém o número e converte para inteiro
                    resp_pos = &recv_line[strlen(resp_expected) + 1];
                    ignore = kstrtol(resp_pos, 10, &resp_number);

                    recv_size = 0;
                    return (int)resp_number;
                }
                else { // Não é a linha que estávamos esperando. Pega a próxima.
                    char error_response[100];
                    sprintf(error_response, "ERR Unknown command.");

                    if(!strncmp(recv_line, error_response, strlen(error_response))) {
                        printk(KERN_INFO "Smartlamp: Houve um erro!\n");
                        recv_size = 0;
                        return -1;
                    }

                    printk(KERN_INFO "SmartLamp: Nao eh resposta para %s! Tentiva %d. Proxima linha...\n", cmd, retries--);
                    recv_size = 0; // Limpa a linha lida (recv_line)
                }
            }
            else { // É um caractere normal (sem ser nova linha), coloca no recv_line e lê o próximo caractere
                if (recv_size < MAX_RECV_LINE - 1) {
                    recv_line[recv_size] = usb_in_buffer[i];
                    recv_size++;
                }
            }
        }
    }
    return -1; // Não recebi a resposta esperada do dispositivo
}
