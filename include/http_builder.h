#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>

// Types de méthodes HTTP
typedef enum {
    HTTP_METHOD_GET_C = 0,
    HTTP_METHOD_POST_C,
    HTTP_METHOD_PUT_C,
    HTTP_METHOD_DELETE_C
} HttpMethod_C;

// Types de contenu
typedef enum {
    CONTENT_TYPE_JSON = 0,
    CONTENT_TYPE_FORM,
    CONTENT_TYPE_TEXT,
    CONTENT_TYPE_BINARY
} ContentType_C;

// Structure de requête HTTP simplifiée
typedef struct {
    char url[256];
    HttpMethod_C method;
    ContentType_C contentType;
    char body[512];
    int timeoutMs;
    bool requiresAuth;
} HttpRequest_C;

// Construction et validation de requêtes
bool Http_BuildRequest_C(const char* url, HttpMethod_C method, const char* body, HttpRequest_C* request);
bool Http_IsValidTimeout_C(int timeoutMs);
bool Http_IsValidContentType_C(ContentType_C type, const char* body);
const char* Http_GetContentTypeString_C(ContentType_C type);

// Utilitaires d'URL
bool Http_IsHttpsRequired_C(const char* url);
bool Http_ExtractDomain_C(const char* url, char* domain, size_t domainSize);

#ifdef __cplusplus
}
#endif
