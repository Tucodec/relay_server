
#ifndef SUV_EXPORT_H_23443546575
#define SUV_EXPORT_H_23443546575

#ifdef WIN32
#ifdef SIMPLE_UV_EXPORTS
#define SUV_EXPORT __declspec(dllexport)
#else
#define SUV_EXPORT __declspec(dllimport)
#endif
#else
#ifdef __GNUC__
#define SUV_EXPORT __attribute__ ((visibility("default")))
#else
#define SUV_EXPORT
#endif
#endif


#endif /* SUV_EXPORT_H_23443546575 */