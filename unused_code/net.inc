

        EXTERN _app_net_SetNetworked, _app_net_GetNetworked
        EXTERN _app_net_Init, _app_net_Uninit, _app_net_CloseSockets, _app_net_ClearCallback
        EXTERN _app_net_MakeServer, _app_net_MakeClient
        EXTERN _app_net_Connect, _app_net_Disconnect
        EXTERN _app_net_ProcessLoop, _app_net_SetupServer, _app_net_SetupClient
        EXTERN _app_net_ReceiveData
        EXTERN _app_net_ServerEvent, _app_net_PlayerEvent, _app_net_EnemyEvent, _app_net_ParticleEvent
        EXTERN _RXbuf, _TXbuf

        EXTERN SERVER_CLI_CONN, SERVER_CLI_DIS, SERVER_SRV_DIS, SERVER_SRV_AUTH
        EXTERN PLAYER_SPAWN, PLAYER_MOVE, PLAYER_DIE
        EXTERN ENEMY_SPAWN, ENEMY_MOVE, ENEMY_DIE
        EXTERN PPE_SPAWN, PPE_MOVE, PPE_DIE

_app_net_SetNetworked_arglen          equ   4
_app_net_SetupClient_arglen           equ   8
_app_net_ServerEvent_arglen           equ   4
_app_net_PlayerEvent_arglen           equ   8
_app_net_EnemyEvent_arglen            equ   8
_app_net_ParticleEvent_arglen         equ   8
