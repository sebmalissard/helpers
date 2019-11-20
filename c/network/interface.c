#include <arpa/inet.h>
#include <asm/types.h>
#include <errno.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <net/if.h>  
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define check_errno(err, msg)                                                                      \
if (err == -1)                                                                                     \
{                                                                                                  \
    printf("%s:%d (%s) %s: %s\n", __FILE__, __LINE__, __FUNCTION__, msg, strerror(errno));         \
    ret = -1;                                                                                      \
    goto exit;                                                                                     \
}


int network_ifup(char *interface)
{
    int ret = 0;
    int rc = 0;
    int sockfd = 0;
    struct ifreq ifr = {0};
    
    strncpy(ifr.ifr_name, interface, IFNAMSIZ);
    
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    check_errno(sockfd, "Fail to create socket");
    
    rc = ioctl(sockfd, SIOCGIFFLAGS, &ifr);
    check_errno(rc, "Fail to request ioctl SIOCGIFFLAGS");
    
    ifr.ifr_flags |= IFF_UP;
    
    rc = ioctl(sockfd, SIOCSIFFLAGS, &ifr);
    check_errno(rc, "Fail to request ioctl SIOCSIFFLAGS");
    
exit:
    if (sockfd)
        close(sockfd);
    
    return ret;
}

int network_ifdown(char *interface)
{
    int ret = 0;
    int rc = 0;
    int sockfd = 0;
    struct ifreq ifr = {0};
    
    strncpy(ifr.ifr_name, interface, IFNAMSIZ);
    
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    check_errno(sockfd, "Fail to create socket");
    
    rc = ioctl(sockfd, SIOCGIFFLAGS, &ifr);
    check_errno(rc, "Fail to request ioctl SIOCGIFFLAGS");
    
    ifr.ifr_flags &= ~IFF_UP;
    
    rc = ioctl(sockfd, SIOCSIFFLAGS, &ifr);
    check_errno(rc, "Fail to request ioctl SIOCSIFFLAGS");
    
exit:
    if (sockfd)
        close(sockfd);
    
    return ret;
}

// TODO other method:
// rtnetlink
// http://iijean.blogspot.com/2010/03/howto-get-list-of-network-interfaces-in.html
int network_iflist()
{
    int ret = 0;
    int rc = 0;
    int sockfd = 0;
    struct ifconf ifc = {0};
    struct ifreq ifr[16] = {{0}}; // Max interfaces listed
    int i = 0;
    char addrbuf[1024];
    
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    check_errno(sockfd, "Fail to create socket");
    
    ifc.ifc_len = sizeof(ifr);
    ifc.ifc_req = ifr;
    
    // SIOCGIFCONF return only up interface with an IP
    ioctl(sockfd, SIOCGIFCONF, &ifc);
    check_errno(rc, "Fail to request ioctl SIOCGIFCONF");
    
    
    for (i=0; i<16; i++)
    {
        printf("%d. %s : %s\n", i, ifr[i].ifr_name, inet_ntop(ifr[i].ifr_addr.sa_family, &((struct sockaddr_in*)&ifr[i].ifr_addr)->sin_addr, addrbuf, sizeof(addrbuf)));
    }
    
exit:
    if (sockfd)
        close(sockfd);
    
    return ret;
}

// TODO Other method get from "/proc/net/dev"
// http://web.archive.org/web/20180522173537/http://www.linuxdevcenter.com/pub/a/linux/2000/11/16/LinuxAdmin.html

#define BUF_SIZE    8192

typedef struct nl_req_s
{
    struct nlmsghdr hdr;
    struct rtgenmsg gen;
} nl_req_t;

int network_iflist2()
{
    int rc = 0;
    int ret = 0;
    int sockfd = 0;
    struct sockaddr_nl local = {0};     /* our local (user space) side of the communication */
    struct sockaddr_nl kernel = {0};    /* the remote (kernel space) side of the communication */
    struct msghdr msg = {0};            /* generic msghdr struct for use with sendmsg */
    struct iovec iov = {0};             /* IO vector for sendmsg and recvmsg */
    int msg_len = 0;
    nl_req_t req = {0};                 /* structure that describes the rtnetlink packet itself */
    pid_t pid = 0;                      /* our process ID to build the correct netlink address */
    struct nlmsghdr *nlmsg = NULL;
    char buf[BUF_SIZE];
    
    pid = getpid();

    local.nl_family = AF_NETLINK;
    local.nl_pid = pid;
    local.nl_groups = RTNLGRP_NONE;

    kernel.nl_family = AF_NETLINK; /* fill-in kernel address (destination of our message) */
    
    req.hdr.nlmsg_len = NLMSG_LENGTH(sizeof(struct rtgenmsg));
    req.hdr.nlmsg_type = RTM_GETLINK;
    req.hdr.nlmsg_flags = NLM_F_REQUEST | NLM_F_DUMP; 
    req.hdr.nlmsg_seq = 1;
    req.hdr.nlmsg_pid = pid;
    req.gen.rtgen_family = AF_PACKET; /* no preferred AF, we will get *all* interfaces */

    iov.iov_base = &req;
    iov.iov_len = req.hdr.nlmsg_len;
    
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_name = &kernel;
    msg.msg_namelen = sizeof(kernel);

    sockfd = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
    check_errno(sockfd, "Fail to create socket");

    rc = bind(sockfd, (struct sockaddr *) &local, sizeof(local));
    check_errno(rc, "Cannot bind, are you root ? if yes, check netlink/rtnetlink kernel support");

    rc = sendmsg(sockfd, (struct msghdr *) &msg, MSG_DONTWAIT);
    check_errno(rc, "Fail to send message");

    iov.iov_base = buf;
    iov.iov_len = BUF_SIZE;

    /* parse reply */
    while (1)
    {
        memset(buf, 0, BUF_SIZE);
        memset(&iov, 0, sizeof(iov));
        memset(&msg, 0, sizeof(msg));
        
        iov.iov_base = buf;
        iov.iov_len = BUF_SIZE;
        
        msg.msg_iov = &iov;
        msg.msg_iovlen = 1;
        msg.msg_name = &kernel;
        msg.msg_namelen = sizeof(kernel);
        
        msg_len = recvmsg(sockfd, &msg, MSG_DONTWAIT);
        check_errno(msg_len, "Fail to receive message");
        
        nlmsg = (struct nlmsghdr *)buf;
        while (NLMSG_OK(nlmsg, msg_len))
        {
            if (nlmsg->nlmsg_type == NLMSG_DONE)
            {
                goto exit;
            }
            
            if (nlmsg->nlmsg_type == RTM_NEWLINK)
            {
                struct ifinfomsg *iface = NLMSG_DATA(nlmsg);
                struct rtattr *attr = IFLA_RTA(iface);
                int len =nlmsg->nlmsg_len - NLMSG_LENGTH(sizeof(*iface));
                
                while (RTA_OK(attr, len))
                {
                    if(attr->rta_type == IFLA_IFNAME)
                    {
                        printf("Interface %d: %s\n", iface->ifi_index, (char *)RTA_DATA(attr));
                    }
                    
                    attr = RTA_NEXT(attr, len);
                }
            }
            
            nlmsg = NLMSG_NEXT(nlmsg, msg_len);
        }
    }
    
exit:
    if (sockfd)
        close(sockfd);

  return 0;
}

int main()
{
    //network_iflist();
    network_iflist2();
}
