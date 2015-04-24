#ifndef _VC_CLIENT_COMMON_H
#define _VC_CLIENT_COMMON_H
struct vc_ops * vc_common_open(struct vc_attr * attr);
struct vc_ops * vc_common_close(struct vc_attr * attr);
struct vc_ops * vc_common_xmit(struct vc_attr * attr);
struct vc_ops * vc_common_ioctl(struct vc_attr * attr);
struct vc_ops * vc_common_recv(struct vc_attr * attr, char *buf, int len);
void vc_common_purge(struct vc_attr * attr, unsigned int pflags);
#endif
