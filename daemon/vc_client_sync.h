#ifndef _VC_CLIENT_SYNC_H
#define _VC_CLIENT_SYNC_H
struct vc_ops * vc_sync_jmp(struct vc_attr *attr, struct vc_ops *current);
struct vc_ops * vc_sync_jmp_recv(struct vc_attr * attr, struct vc_ops *, char *buf, int len);
#endif
