// User-level IPC library routines

#include <inc/lib.h>

// Receive a value via IPC and return it.
// If 'pg' is nonnull, then any page sent by the sender will be mapped at
//	that address.
// If 'from_env_store' is nonnull, then store the IPC sender's envid in
//	*from_env_store.
// If 'perm_store' is nonnull, then store the IPC sender's page permission
//	in *perm_store (this is nonzero iff a page was successfully
//	transferred to 'pg').
// If the system call fails, then store 0 in *fromenv and *perm (if
//	they're nonnull) and return the error.
// Otherwise, return the value sent by the sender
//
// Hint:
//   Use 'thisenv' to discover the value and who sent it.
//   If 'pg' is null, pass sys_ipc_recv a value that it will understand
//   as meaning "no page".  (Zero is not the right value, since that's
//   a perfectly valid place to map a page.)
int32_t
ipc_recv(envid_t *from_env_store, void *pg, int *perm_store)
{
	// LAB 4: Your code here.
	// panic("ipc_recv not implemented");
        int r;
        
        if (pg == NULL) /// 如果pg == null，说明只使用env结构中的一个32位整数做消息载体
                r = sys_ipc_recv((void *)UTOP);
        else    // 否则使用一个页传递消息
                r = sys_ipc_recv(pg);
        if (from_env_store != NULL) // 使用env结构中的env_ipc_from属性查看是哪个进程传递信息的
                *from_env_store = r < 0 ? 0 : thisenv->env_ipc_from;
        if (perm_store != NULL)     // 使用env结构中的env_ipc_from属性查看页面映射的权限
                *perm_store = r < 0 ? 0 : thisenv->env_ipc_perm;
        if (r < 0)
                return r;
        else
                return thisenv->env_ipc_value;
}

// Send 'val' (and 'pg' with 'perm', if 'pg' is nonnull) to 'toenv'.
// This function keeps trying until it succeeds.
// It should panic() on any error other than -E_IPC_NOT_RECV.
//
// Hint:
//   Use sys_yield() to be CPU-friendly.
//   If 'pg' is null, pass sys_ipc_try_send a value that it will understand
//   as meaning "no page".  (Zero is not the right value.)
void
ipc_send(envid_t to_env, uint32_t val, void *pg, int perm)
{
        int r;
        void *dstpg;

        dstpg = pg != NULL ? pg : (void *)UTOP;
        // 等待对端接受
        while((r = sys_ipc_try_send(to_env, val, dstpg, perm)) < 0) {
                if (r != -E_IPC_NOT_RECV)
                        panic("ipc_send: send message error %e", r);
                // 注意，由于未
                sys_yield();
        }
	// panic("ipc_send not implemented");
}

// Find the first environment of the given type.  We'll use this to
// find special environments.
// Returns 0 if no such environment exists.
envid_t
ipc_find_env(enum EnvType type)
{
	int i;
	for (i = 0; i < NENV; i++)
		if (envs[i].env_type == type)
			return envs[i].env_id;
	return 0;
}
