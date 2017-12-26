
#ifndef CLIENT_NETWORK_EXPORT_H_23443313543
#define CLIENT_NETWORK_EXPORT_H_23443313543

#ifdef WIN32
#ifdef CLIENT_NETWORK_EXPORTS
#define CLIENT_NETWORK_EXPORT __declspec(dllexport)
#else
#define CLIENT_NETWORK_EXPORT __declspec(dllimport)
#endif
#else
#ifdef __GNUC__
#define CLIENT_NETWORK_EXPORT __attribute__ ((visibility("default")))
#else
#define CLIENT_NETWORK_EXPORT
#endif
#endif


#endif /* CLIENT_NETWORK_EXPORT_H_23443313543 */