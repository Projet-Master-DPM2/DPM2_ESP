#include "../../include/http_builder.h"
#include <string.h>
#include <stdio.h>

bool Http_BuildRequest_C(const char* url, HttpMethod_C method, const char* body, HttpRequest_C* request) {
    if (!url || !request) return false;
    
    // Vérifier la longueur de l'URL
    size_t urlLen = strlen(url);
    if (urlLen == 0 || urlLen >= sizeof(request->url)) return false;
    
    // Copier l'URL
    strncpy(request->url, url, sizeof(request->url) - 1);
    request->url[sizeof(request->url) - 1] = '\0';
    
    // Définir la méthode
    request->method = method;
    
    // Gérer le body
    if (body && strlen(body) > 0) {
        size_t bodyLen = strlen(body);
        if (bodyLen >= sizeof(request->body)) return false;
        strncpy(request->body, body, sizeof(request->body) - 1);
        request->body[sizeof(request->body) - 1] = '\0';
        
        // Détecter le type de contenu automatiquement
        if (body[0] == '{' || body[0] == '[') {
            request->contentType = CONTENT_TYPE_JSON;
        } else if (strstr(body, "=") != NULL && strstr(body, "&") != NULL) {
            request->contentType = CONTENT_TYPE_FORM;
        } else {
            request->contentType = CONTENT_TYPE_TEXT;
        }
    } else {
        request->body[0] = '\0';
        request->contentType = CONTENT_TYPE_TEXT;
    }
    
    // Paramètres par défaut
    request->timeoutMs = 8000; // 8 secondes
    request->requiresAuth = Http_IsHttpsRequired_C(url);
    
    return true;
}

bool Http_IsValidTimeout_C(int timeoutMs) {
    return (timeoutMs >= 1000 && timeoutMs <= 60000); // 1s à 60s
}

bool Http_IsValidContentType_C(ContentType_C type, const char* body) {
    if (!body || strlen(body) == 0) return true; // Pas de body = OK
    
    switch (type) {
        case CONTENT_TYPE_JSON:
            // Vérification basique JSON
            return (body[0] == '{' || body[0] == '[');
            
        case CONTENT_TYPE_FORM:
            // Doit contenir au moins un '='
            return (strstr(body, "=") != NULL);
            
        case CONTENT_TYPE_TEXT:
        case CONTENT_TYPE_BINARY:
            return true; // Toujours valide
            
        default:
            return false;
    }
}

const char* Http_GetContentTypeString_C(ContentType_C type) {
    switch (type) {
        case CONTENT_TYPE_JSON:
            return "application/json";
        case CONTENT_TYPE_FORM:
            return "application/x-www-form-urlencoded";
        case CONTENT_TYPE_TEXT:
            return "text/plain";
        case CONTENT_TYPE_BINARY:
            return "application/octet-stream";
        default:
            return "text/plain";
    }
}

bool Http_IsHttpsRequired_C(const char* url) {
    if (!url) return false;
    
    // Heuristiques pour déterminer si HTTPS est requis
    if (strstr(url, "api.") != NULL) return true;      // APIs
    if (strstr(url, "secure") != NULL) return true;    // URLs avec "secure"
    if (strstr(url, "login") != NULL) return true;     // Pages de login
    if (strstr(url, "payment") != NULL) return true;   // Paiements
    if (strstr(url, "bank") != NULL) return true;      // Services bancaires
    
    return false; // Par défaut, HTTP OK
}

bool Http_ExtractDomain_C(const char* url, char* domain, size_t domainSize) {
    if (!url || !domain || domainSize < 8) return false;
    
    const char* start = url;
    
    // Ignorer le protocole
    if (strncmp(url, "http://", 7) == 0) {
        start = url + 7;
    } else if (strncmp(url, "https://", 8) == 0) {
        start = url + 8;
    }
    
    // Trouver la fin du domaine
    const char* end = strchr(start, '/');
    if (!end) end = start + strlen(start);
    
    size_t domainLen = end - start;
    if (domainLen == 0 || domainLen >= domainSize) return false;
    
    strncpy(domain, start, domainLen);
    domain[domainLen] = '\0';
    
    return true;
}
