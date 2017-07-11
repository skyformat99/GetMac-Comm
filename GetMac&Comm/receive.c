#include <stdio.h>  
#define _WIN64
#include <stdlib.h>  
#include <pcap.h>  
  
  
char *iptos(u_long in);       //u_long��Ϊ unsigned long  
void packet_handler(u_char *param, const struct pcap_pkthdr *header, const u_char *pkt_data);  
//struct tm *ltime;                 //��ʱ�䴦���йصı���  
  
struct IpAddress  
{  
    u_char byte1;  
    u_char byte2;  
    u_char byte3;  
    u_char byte4;  
};  
  
//֡ͷ���ṹ�壬��14�ֽ�  
struct EthernetHeader  
{  
    u_char DestMAC[6];    //Ŀ��MAC��ַ 6�ֽ�  
    u_char SourMAC[6];   //ԴMAC��ַ 6�ֽ�  
    u_short EthType;         //��һ��Э�����ͣ���0x0800������һ����IPЭ�飬0x0806Ϊarp  2�ֽ�  
};  
  
//IPͷ���ṹ�壬��20�ֽ�  
struct IpHeader  
{  
    unsigned char Version_HLen;   //�汾��Ϣ4λ ��ͷ����4λ 1�ֽ�  
    unsigned char TOS;                 //��������    1�ֽ�  
    short Length;                              //���ݰ����� 2�ֽ�  
    short Ident;                                 //���ݰ���ʶ  2�ֽ�  
    short Flags_Offset;                    //��־3λ��Ƭƫ��13λ  2�ֽ�  
    unsigned char TTL;                    //���ʱ��  1�ֽ�  
    unsigned char Protocol;           //Э������  1�ֽ�  
    short Checksum;                        //�ײ�У��� 2�ֽ�  
    struct IpAddress SourceAddr;           //ԴIP��ַ   4�ֽ�  
    struct IpAddress DestinationAddr;   //Ŀ��IP��ַ  4�ֽ�  
};  
  
//TCPͷ���ṹ�壬��20�ֽ�  
struct TcpHeader  
{  
    unsigned short SrcPort;                        //Դ�˿ں�  2�ֽ�  
    unsigned short DstPort;                        //Ŀ�Ķ˿ں� 2�ֽ�  
    unsigned int SequenceNum;               //���  4�ֽ�  
    unsigned int Acknowledgment;         //ȷ�Ϻ�  4�ֽ�  
    unsigned char HdrLen;                         //�ײ�����4λ������λ6λ ��10λ  
    unsigned char Flags;                              //��־λ6λ  
    unsigned short AdvertisedWindow;  //���ڴ�С16λ 2�ֽ�  
    unsigned short Checksum;                  //У���16λ   2�ֽ�  
    unsigned short UrgPtr;                        //����ָ��16λ   2�ֽ�  
};  
  
//TCPα�ײ��ṹ�� 12�ֽ�  
struct PsdTcpHeader  
{  
    unsigned long SourceAddr;                     //ԴIP��ַ  4�ֽ�  
    unsigned long DestinationAddr;             //Ŀ��IP��ַ 4�ֽ�  
    char Zero;                                                    //���λ  1�ֽ�  
    char Protcol;                                               //Э���  1�ֽ�  
    unsigned short TcpLen;                           //TCP������ 2�ֽ�  
};  
  
  
int main(){  
  
    struct EthernetHeader *ethernet;    //��̫��֡ͷ  
    struct IpHeader *ip;                            //IPͷ  
    struct TcpHeader *tcp;                      //TCPͷ  
    struct PsdTcpHeader *ptcp;             //TCPα�ײ�  
  
    struct pcap_if  *alldevs;       //��������������  
    struct pcap_if  *d;                  //ѡ�е�����������  
    char errbuf[PCAP_ERRBUF_SIZE];   //���󻺳���,��СΪ256  
    char source[PCAP_ERRBUF_SIZE];  
    struct pcap_t *adhandle;           //��׽ʵ��,��pcap_open���صĶ���  
    int i = 0;                            //��������������  
    struct pcap_pkthdr *header;    //���յ������ݰ���ͷ��  
    const u_char *pkt_data;           //���յ������ݰ�������  
    int res;                                    //��ʾ�Ƿ���յ������ݰ�  
    u_int netmask;                       //����ʱ�õ���������  
    char packet_filter[] = "tcp";        //�����ַ�  
    struct bpf_program fcode;                     //pcap_compile�����õĽṹ��  
  
    u_int ip_len;                                       //ip��ַ��Ч����  
    u_short sport,dport;                        //�����ֽ�����  
    u_char packet[100];                       //�������ݰ�Ŀ�ĵ�ַ  
    struct pcap_dumper_t *dumpfile;         //���ļ�  
	struct pcap_addr *a;       //�����������ĵ�ַ�����洢����
	int num;  
	int inum; 
  
    //time_t local_tv_sec;              //��ʱ�䴦���йصı���  
    //char timestr[16];                 //��ʱ�䴦���йصı���  
  
      
    //��ȡ�����������б�  
    if(pcap_findalldevs_ex(PCAP_SRC_IF_STRING,NULL,&alldevs,errbuf) == -1){  
        //���Ϊ-1������ֻ�ȡ�������б�ʧ��  
        fprintf(stderr,"Error in pcap_findalldevs_ex:\n",errbuf);  
        //exit(0)���������˳�,exit(other)Ϊ�������˳�,���ֵ�ᴫ������ϵͳ  
        exit(1);  
    }  
    //��ӡ�豸�б���Ϣ  
    for(d = alldevs;d !=NULL;d = d->next){  
        printf("-----------------------------------------------------------------\nnumber:%d\nname:%s\n",++i,d->name);  
        if(d->description){  
            //��ӡ��������������Ϣ  
            printf("description:%s\n",d->description);  
        }else{  
            //������������������Ϣ  
            printf("description:%s","no description\n");  
        }  
        //��ӡ���ػ��ص�ַ  
        printf("\tLoopback: %s\n",(d->flags & PCAP_IF_LOOPBACK)?"yes":"no");  
          
        for(a = d->addresses;a;a = a->next){  
             //sa_family�����˵�ַ������,��IPV4��ַ���ͻ���IPV6��ַ����  
             switch (a->addr->sa_family)  
             {  
                 case AF_INET:  //����IPV4���͵�ַ  
                     printf("Address Family Name:AF_INET\n");  
                     if(a->addr){  
                         //->�����ȼ���ͬ������,����ǿ������ת��,��ΪaddrΪsockaddr���ͣ�������в�����ת��Ϊsockaddr_in����  
                         printf("Address:%s\n",iptos(((struct sockaddr_in *)a->addr)->sin_addr.s_addr));  
                     }  
                    if (a->netmask){  
                         printf("\tNetmask: %s\n",iptos(((struct sockaddr_in *)a->netmask)->sin_addr.s_addr));  
                    }  
                    if (a->broadaddr){  
                           printf("\tBroadcast Address: %s\n",iptos(((struct sockaddr_in *)a->broadaddr)->sin_addr.s_addr));  
                     }  
                     if (a->dstaddr){  
                           printf("\tDestination Address: %s\n",iptos(((struct sockaddr_in *)a->dstaddr)->sin_addr.s_addr));  
                     }  
                     break;  
                 case AF_INET6: //����IPV6���͵�ַ  
                     printf("Address Family Name:AF_INET6\n");  
                     printf("this is an IPV6 address\n");  
                     break;  
                 default:  
                     break;  
             }  
         }  
    }  
    //iΪ0��������ѭ��δ����,��û���ҵ�������,���ܵ�ԭ��ΪWinpcapû�а�װ����δɨ�赽  
    if(i == 0){  
        printf("interface not found,please check winpcap installation");  
    }  
  
    printf("Enter the interface number(1-%d):",i);  
    //���û�ѡ��ѡ���ĸ�����������ץ��  
    scanf("%d",&num);  
    printf("\n");  
  
    //�û���������ֳ�������Χ  
    if(num<1||num>i){  
        printf("number out of range\n");  
        pcap_freealldevs(alldevs);  
        return -1;  
    }  
    //��ת��ѡ�е�������  
    for(d=alldevs, i=0; i< num-1 ; d=d->next, i++);  
  
    //���е��˴�˵���û��������ǺϷ���  
    if((adhandle = pcap_open(d->name,        //�豸����  
                                                        65535,       //������ݰ������ݳ���  
                                                        PCAP_OPENFLAG_PROMISCUOUS,  //����ģʽ  
                                                        1000,           //��ʱʱ��  
                                                        NULL,          //Զ����֤  
                                                        errbuf         //���󻺳�  
                                                        )) == NULL){  
        //��������ʧ��,��ӡ�����ͷ��������б�  
        fprintf(stderr,"\nUnable to open the adapter. %s is not supported by WinPcap\n", d->name);  
        // �ͷ��豸�б�   
        pcap_freealldevs(alldevs);  
        return -1;  
    }  
      
  
    //��ӡ���,���ڼ�����  
    printf("\nlistening on %s...\n", d->description);  
  
    //�������粻����̫��,�˴�ֻȡ�������  
    if(pcap_datalink(adhandle) != DLT_EN10MB)  
    {  
        fprintf(stderr,"\nThis program works only on Ethernet networks.\n");  
        //�ͷ��б�  
        pcap_freealldevs(alldevs);  
        return -1;  
    }  
  
    //�Ȼ�õ�ַ����������  
    if(d->addresses != NULL)  
        //��ýӿڵ�һ����ַ������   
        netmask=((struct sockaddr_in *)(d->addresses->netmask))->sin_addr.S_un.S_addr;  
    else  
        // ����ӿ�û�е�ַ����ô���Ǽ���һ��C�������  
        netmask=0xffffff;  
  
    //pcap_compile()��ԭ���ǽ��߲�Ĳ������˱�  
    //��ʽ������ܹ����������������͵ĵͲ���ֽ���  
    if(pcap_compile(adhandle,   //�������������  
                                        &fcode,  
                                        packet_filter,   //����ip��UDP  
                                        1,                       //�Ż���־  
                                        netmask           //��������  
                                        )<0)  
    {  
        //���˳�������  
        fprintf(stderr,"\nUnable to compile the packet filter. Check the syntax.\n");  
        // �ͷ��豸�б�  
        pcap_freealldevs(alldevs);  
        return -1;  
    }  
  
    //���ù�����  
    if (pcap_setfilter(adhandle, &fcode)<0)  
    {  
        fprintf(stderr,"\nError setting the filter.\n");  
        //�ͷ��豸�б�  
        pcap_freealldevs(alldevs);  
        return -1;  
    }  
  
  
    //����pcap_next_ex���������ݰ�  
    while((res = pcap_next_ex(adhandle,&header,&pkt_data))>=0)  
    {  
        if(res ==0){  
            //����ֵΪ0����������ݰ���ʱ������ѭ����������  
            continue;  
        }else{  
			char * data; 
            //���е��˴�������ܵ����������ݰ�  
            //headerΪ֡��ͷ��  
            printf("%.6ld len:%d ", header->ts.tv_usec, header->len);  
            // ���IP���ݰ�ͷ����λ��  
            ip = (struct IpHeader *) (pkt_data +14);    //14Ϊ��̫��֡ͷ������  
            //���TCPͷ����λ��  
            ip_len = (ip->Version_HLen & 0xf) *4;  
            printf("ip_length:%d ",ip_len);  
            tcp = (struct TcpHeader *)((u_char *)ip+ip_len);  
            
            data = (char *)((u_char *)tcp+20);  
            //�������ֽ�����ת���������ֽ�����  
            sport = ntohs( tcp->SrcPort );  
            dport = ntohs( tcp->DstPort );  
            printf("srcport:%d desport:%d\n",sport,dport);  
            printf("%d.%d.%d.%d.%d -> %d.%d.%d.%d.%d\n",  
                    ip->SourceAddr.byte1,  
                    ip->SourceAddr.byte2,  
                    ip->SourceAddr.byte3,  
                    ip->SourceAddr.byte4,  
                    sport,  
                    ip->DestinationAddr.byte1,  
                    ip->DestinationAddr.byte2,  
                    ip->DestinationAddr.byte3,  
                    ip->DestinationAddr.byte4,  
                    dport);  
            printf("%s\n",data);  
        }  
  
    }  
  
      
    //�ͷ������������б�  
    pcap_freealldevs(alldevs);  
  
    /** 
    int pcap_loop  ( pcap_t *  p,   
                                  int  cnt,   
                                  pcap_handler  callback,   
                                  u_char *  user    
                                 ); 
     typedef void (*pcap_handler)(u_char *, const struct pcap_pkthdr *, 
                 const u_char *); 
    */  
    //��ʼ������Ϣ,���������ݰ�ʱ,���Զ������������  
    //pcap_loop(adhandle,0,packet_handler,NULL);  
   
    scanf("%d", &inum);  
  
    return 0;  
  
}  
  
/* ÿ�β������ݰ�ʱ��libpcap�����Զ���������ص����� */  
/** 
pcap_loop()�����ǻ��ڻص���ԭ�����������ݲ���ģ��缼���ĵ���˵������һ�־���ķ�����������ĳЩ�����£� 
����һ�ֺܺõ�ѡ�񡣵����ڴ���ص���ʱ��Ტ��ʵ�ã��������ӳ���ĸ��Ӷȣ��ر����ڶ��̵߳�C++������ 
*/  
/* 
void packet_handler(u_char *param, const struct pcap_pkthdr *header, const u_char *pkt_data) 
{ 
    struct tm *ltime = NULL; 
    char timestr[16]; 
    time_t local_tv_sec; 
 
    // ��ʱ���ת���ɿ�ʶ��ĸ�ʽ 
    local_tv_sec = header->ts.tv_sec; 
    localtime_s(ltime,&local_tv_sec); 
    strftime( timestr, sizeof timestr, "%H:%M:%S", ltime); 
 
    printf("%s,%.6ld len:%d\n", timestr, header->ts.tv_usec, header->len); 
 
} 
*/  
/* ���������͵�IP��ַת�����ַ������͵� */  
#define IPTOSBUFFERS    12  
char *iptos(u_long in)  
{  
    static char output[IPTOSBUFFERS][3*4+3+1];  
    static short which;  
    u_char *p;  
  
    p = (u_char *)&in;  
    which = (which + 1 == IPTOSBUFFERS ? 0 : which + 1);  
    sprintf(output[which], "%d.%d.%d.%d", p[0], p[1], p[2], p[3]);  
    return output[which];  
}  