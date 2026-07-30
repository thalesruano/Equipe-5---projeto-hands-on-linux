#include <linux/module.h>
#include <linux/usb.h>
#include <linux/slab.h>

MODULE_AUTHOR("DevTITANS <devtitans@icomp.ufam.edu.br>");
MODULE_DESCRIPTION("Driver de acesso ao SmartLamp (ESP32 com Chip Serial CP2102)");
MODULE_LICENSE("GPL");

#define MAX_RECV_LINE 100 // Tamanho máximo de uma linha de resposta do dispositivo USB

static char recv_line[MAX_RECV_LINE];            // Buffer para armazenar linha completa recebida
static struct usb_device *smartlamp_device;      // Referência para o dispositivo USB
static uint usb_in, usb_out;                     // Endereços das portas de entrada e saída da USB
static char *usb_in_buffer, *usb_out_buffer;     // Buffers de entrada e saída da USB
static int usb_max_size;                         // Tamanho máximo de uma mensagem USB

#define VENDOR_ID 0x10c4   /* VendorID do CP2102 */
#define PRODUCT_ID 0xea60  /* ProductID do CP2102 */
static const struct usb_device_id id_table[] = { { USB_DEVICE(VENDOR_ID, PRODUCT_ID) }, {} };

static int  usb_probe(struct usb_interface *ifce, const struct usb_device_id *id);
static void usb_disconnect(struct usb_interface *ifce);
static int  usb_write_serial(char *cmd, int param);
static int  usb_read_serial(char *cmd);

static ssize_t attr_show(struct kobject *sys_obj, struct kobj_attribute *attr, char *buff);
static ssize_t attr_store(struct kobject *sys_obj, struct kobj_attribute *attr, const char *buff, size_t count);

static struct kobj_attribute  led_attribute       = __ATTR(led, S_IRUGO | S_IWUSR, attr_show, attr_store);
static struct kobj_attribute  ldr_attribute       = __ATTR(ldr, S_IRUGO | S_IWUSR, attr_show, attr_store);
static struct kobj_attribute  threshold_attribute = __ATTR(threshold, S_IRUGO | S_IWUSR, attr_show, attr_store);
static struct attribute      *attrs[]             = { &led_attribute.attr, &ldr_attribute.attr, &threshold_attribute.attr, NULL };
static struct attribute_group attr_group          = { .attrs = attrs };
static struct kobject        *sys_obj;

// Configuração do CP2102
static int smartlamp_config_serial(struct usb_device *dev)
{
    int ret;
    u32 baudrate = 9600;

    printk(KERN_INFO "SmartLamp: Configurando a porta serial...\n");

    // 1. Habilita a interface UART do CP2102
    ret = usb_control_msg(dev, usb_sndctrlpipe(dev, 0),
                          0x00, 0x41, 0x0001, 0, NULL, 0, 1000);
    if (ret) {
        printk(KERN_ERR "SmartLamp: Erro ao habilitar a UART (código %d)\n", ret);
        return ret;
    }

    // 2. Define o baud rate
    ret = usb_control_msg(dev, usb_sndctrlpipe(dev, 0),
                          0x1E, 0x41, 0, 0, &baudrate, sizeof(baudrate), 1000);
    if (ret < 0) {
        printk(KERN_ERR "SmartLamp: Erro ao configurar o baud rate (código %d)\n", ret);
        return ret;
    }

    printk(KERN_INFO "SmartLamp: Baud rate configurado para %d\n", baudrate);
    return 0;
}

MODULE_DEVICE_TABLE(usb, id_table);

static struct usb_driver smartlamp_driver = {
    .name        = "smartlamp",
    .probe       = usb_probe,
    .disconnect  = usb_disconnect,
    .id_table    = id_table,
};

module_usb_driver(smartlamp_driver);

static int usb_probe(struct usb_interface *interface, const struct usb_device_id *id) {
    struct usb_endpoint_descriptor *usb_endpoint_in, *usb_endpoint_out;
    int ret;

    printk(KERN_INFO "SmartLamp: Dispositivo conectado ...\n");

    smartlamp_device = interface_to_usbdev(interface);

    // Aloca e encontra endpoints
    ret = usb_find_common_endpoints(interface->cur_altsetting, &usb_endpoint_in, &usb_endpoint_out, NULL, NULL);
    if (ret) {
        printk(KERN_ERR "SmartLamp: Falha ao localizar endpoints USB\n");
        return ret;
    }

    usb_max_size = usb_endpoint_maxp(usb_endpoint_in);
    usb_in = usb_endpoint_in->bEndpointAddress;
    usb_out = usb_endpoint_out->bEndpointAddress;

    usb_in_buffer = kmalloc(usb_max_size, GFP_KERNEL);
    usb_out_buffer = kmalloc(usb_max_size, GFP_KERNEL);

    if (!usb_in_buffer || !usb_out_buffer) {
        kfree(usb_in_buffer);
        kfree(usb_out_buffer);
        return -ENOMEM;
    }

    ret = smartlamp_config_serial(smartlamp_device);
    if (ret) {
        printk(KERN_ERR "SmartLamp: Falha na configuração da serial\n");
        kfree(usb_in_buffer);
        kfree(usb_out_buffer);
        return ret;
    }

    sys_obj = kobject_create_and_add("smartlamp", kernel_kobj);
    if (!sys_obj) {
        kfree(usb_in_buffer);
        kfree(usb_out_buffer);
        return -ENOMEM;
    }

    ret = sysfs_create_group(sys_obj, &attr_group);
    if (ret) {
        printk(KERN_ERR "SmartLamp: Erro ao criar arquivos no sysfs\n");
        kobject_put(sys_obj);
        kfree(usb_in_buffer);
        kfree(usb_out_buffer);
        return ret;
    }

    return 0;
}

static void usb_disconnect(struct usb_interface *interface) {
    printk(KERN_INFO "SmartLamp: Dispositivo desconectado.\n");

    if (sys_obj) {
        sysfs_remove_group(sys_obj, &attr_group);
        kobject_put(sys_obj);
    }

    kfree(usb_in_buffer);
    kfree(usb_out_buffer);
}

static int usb_write_serial(char *cmd, int param) {
    int ret;
    int actual_size;
    int len;

    if (param < 0) {
        len = snprintf(usb_out_buffer, usb_max_size, "%s\n", cmd);
    } else {
        len = snprintf(usb_out_buffer, usb_max_size, "%s %d\n", cmd, param);
    }

    printk(KERN_INFO "SmartLamp: Enviando comando: %s", usb_out_buffer);

    ret = usb_bulk_msg(
        smartlamp_device,
        usb_sndbulkpipe(smartlamp_device, usb_out),
                       usb_out_buffer,
                       len,
                       &actual_size,
                       1000
    );

    if (ret) {
        printk(KERN_ERR "SmartLamp: Erro ao enviar comando (%d)\n", ret);
        return ret;
    }

    return 0;
}

static int usb_read_serial(char *cmd) {
    int ret, actual_size;
    int recv_size = 0;
    int i;
    int retries = 10;
    char resp_expected[MAX_RECV_LINE];
    char *resp_pos;
    long resp_number = -1;

    snprintf(resp_expected, MAX_RECV_LINE, "RES %s", cmd);

    while (retries > 0) {
        ret = usb_bulk_msg(
            smartlamp_device,
            usb_rcvbulkpipe(smartlamp_device, usb_in),
                           usb_in_buffer,
                           min(usb_max_size, MAX_RECV_LINE),
                           &actual_size,
                           2000
        );

        if (ret) {
            retries--;
            continue;
        }

        for (i = 0; i < actual_size; i++) {
            if (usb_in_buffer[i] == '\n') {
                recv_line[recv_size] = '\0';
                printk(KERN_INFO "SmartLamp: Recebido: '%s'\n", recv_line);

                if (!strncmp(recv_line, resp_expected, strlen(resp_expected))) {
                    resp_pos = &recv_line[strlen(resp_expected) + 1];
                    kstrtol(resp_pos, 10, &resp_number);
                    return (int)resp_number;
                } else {
                    recv_size = 0;
                }
            } else {
                if (recv_size < MAX_RECV_LINE - 1) {
                    recv_line[recv_size] = usb_in_buffer[i];
                    recv_size++;
                }
            }
        }
        retries--;
    }

    return -1;
}

static ssize_t attr_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf) {
    // Verifica se o atributo acessado é o 'led'
    if (strcmp(attr->attr.name, "led") == 0) {
        return sysfs_emit(buf, "Jackeline_Abel_Kevs_Elizeu_Thales_Jhonathas_somos_A_Equipe-5---projeto-hands-on-linux\n"); // Substitua 'Seu Nome Aqui' pelo seu nome
    }
    // Verifica se o atributo acessado é o 'ldr'
    else if (strcmp(attr->attr.name, "ldr") == 0) {
        return sysfs_emit(buf, "DevTITANS\n");
    }

    return -EACCES;
}

static ssize_t attr_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count) {
    int ret, value;

    // Se a tentativa for de escrever no 'ldr', retorna erro (ex: -EACCES)
    if (strcmp(attr->attr.name, "ldr") == 0) {
        return -EACCES;
    }

    // Se for o 'led', processa o valor escrito
    if (strcmp(attr->attr.name, "led") == 0) {
        // Converte a string recebida no buffer para um inteiro
        ret = kstrtoint(buf, 10, &value);
        if (ret < 0) {
            return ret; // Caso a conversão falhe
        }

        // Exibe o valor no log do kernel (dmesg)
        printk(KERN_INFO "Smartlamp: Valor do LED alterado para %d\n", value);

        // ATENÇÃO: É fundamental retornar 'count' no sucesso!
        return count;
    }

    return -EACCES;
}
