#include <unity.h>
#include "../../include/http_builder.h"
#include <string.h>

void setUp(void) {}
void tearDown(void) {}

// Tests de construction de requête
void test_build_request_get() {
    HttpRequest_C request;
    bool result = Http_BuildRequest_C("http://example.com/api", HTTP_METHOD_GET_C, nullptr, &request);
    
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("http://example.com/api", request.url);
    TEST_ASSERT_EQUAL(HTTP_METHOD_GET_C, request.method);
    TEST_ASSERT_EQUAL_STRING("", request.body);
    TEST_ASSERT_EQUAL(8000, request.timeoutMs);
    TEST_ASSERT_FALSE(request.requiresAuth); // Ne contient pas "api."
}

void test_build_request_post_json() {
    HttpRequest_C request;
    const char* jsonBody = "{\"user\":\"test\",\"action\":\"login\"}";
    bool result = Http_BuildRequest_C("https://api.example.com/login", HTTP_METHOD_POST_C, jsonBody, &request);
    
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(HTTP_METHOD_POST_C, request.method);
    TEST_ASSERT_EQUAL_STRING(jsonBody, request.body);
    TEST_ASSERT_EQUAL(CONTENT_TYPE_JSON, request.contentType);
    TEST_ASSERT_TRUE(request.requiresAuth); // HTTPS + api + login
}

void test_build_request_post_form() {
    HttpRequest_C request;
    const char* formBody = "username=test&password=secret";
    bool result = Http_BuildRequest_C("http://example.com/submit", HTTP_METHOD_POST_C, formBody, &request);
    
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING(formBody, request.body);
    TEST_ASSERT_EQUAL(CONTENT_TYPE_FORM, request.contentType);
}

void test_build_request_invalid() {
    HttpRequest_C request;
    
    // URL nulle
    TEST_ASSERT_FALSE(Http_BuildRequest_C(nullptr, HTTP_METHOD_GET_C, nullptr, &request));
    
    // Request nulle
    TEST_ASSERT_FALSE(Http_BuildRequest_C("http://example.com", HTTP_METHOD_GET_C, nullptr, nullptr));
    
    // URL vide
    TEST_ASSERT_FALSE(Http_BuildRequest_C("", HTTP_METHOD_GET_C, nullptr, &request));
    
    // URL trop longue
    char longUrl[300];
    memset(longUrl, 'a', sizeof(longUrl) - 8);
    strcpy(longUrl, "http://");
    memset(longUrl + 7, 'a', sizeof(longUrl) - 8);
    longUrl[sizeof(longUrl) - 1] = '\0';
    TEST_ASSERT_FALSE(Http_BuildRequest_C(longUrl, HTTP_METHOD_GET_C, nullptr, &request));
}

// Tests de validation timeout
void test_timeout_validation() {
    TEST_ASSERT_TRUE(Http_IsValidTimeout_C(1000));   // 1s
    TEST_ASSERT_TRUE(Http_IsValidTimeout_C(8000));   // 8s
    TEST_ASSERT_TRUE(Http_IsValidTimeout_C(30000));  // 30s
    TEST_ASSERT_TRUE(Http_IsValidTimeout_C(60000));  // 60s
    
    TEST_ASSERT_FALSE(Http_IsValidTimeout_C(500));   // Trop court
    TEST_ASSERT_FALSE(Http_IsValidTimeout_C(999));   // Trop court
    TEST_ASSERT_FALSE(Http_IsValidTimeout_C(60001)); // Trop long
    TEST_ASSERT_FALSE(Http_IsValidTimeout_C(120000)); // Trop long
}

// Tests de validation type de contenu
void test_content_type_validation() {
    // JSON valide
    TEST_ASSERT_TRUE(Http_IsValidContentType_C(CONTENT_TYPE_JSON, "{\"test\":true}"));
    TEST_ASSERT_TRUE(Http_IsValidContentType_C(CONTENT_TYPE_JSON, "[1,2,3]"));
    TEST_ASSERT_FALSE(Http_IsValidContentType_C(CONTENT_TYPE_JSON, "not json"));
    
    // Form valide
    TEST_ASSERT_TRUE(Http_IsValidContentType_C(CONTENT_TYPE_FORM, "key=value"));
    TEST_ASSERT_TRUE(Http_IsValidContentType_C(CONTENT_TYPE_FORM, "a=1&b=2"));
    TEST_ASSERT_FALSE(Http_IsValidContentType_C(CONTENT_TYPE_FORM, "no equals"));
    
    // Text et Binary toujours valides
    TEST_ASSERT_TRUE(Http_IsValidContentType_C(CONTENT_TYPE_TEXT, "any text"));
    TEST_ASSERT_TRUE(Http_IsValidContentType_C(CONTENT_TYPE_BINARY, "binary data"));
    
    // Body vide toujours valide
    TEST_ASSERT_TRUE(Http_IsValidContentType_C(CONTENT_TYPE_JSON, ""));
    TEST_ASSERT_TRUE(Http_IsValidContentType_C(CONTENT_TYPE_JSON, nullptr));
}

// Tests de chaînes de type de contenu
void test_content_type_strings() {
    TEST_ASSERT_EQUAL_STRING("application/json", Http_GetContentTypeString_C(CONTENT_TYPE_JSON));
    TEST_ASSERT_EQUAL_STRING("application/x-www-form-urlencoded", Http_GetContentTypeString_C(CONTENT_TYPE_FORM));
    TEST_ASSERT_EQUAL_STRING("text/plain", Http_GetContentTypeString_C(CONTENT_TYPE_TEXT));
    TEST_ASSERT_EQUAL_STRING("application/octet-stream", Http_GetContentTypeString_C(CONTENT_TYPE_BINARY));
    
    // Type invalide
    TEST_ASSERT_EQUAL_STRING("text/plain", Http_GetContentTypeString_C((ContentType_C)999));
}

// Tests de détection HTTPS requis
void test_https_required_detection() {
    TEST_ASSERT_TRUE(Http_IsHttpsRequired_C("http://api.example.com"));
    TEST_ASSERT_TRUE(Http_IsHttpsRequired_C("http://secure.example.com"));
    TEST_ASSERT_TRUE(Http_IsHttpsRequired_C("http://example.com/login"));
    TEST_ASSERT_TRUE(Http_IsHttpsRequired_C("http://payment.service.com"));
    TEST_ASSERT_TRUE(Http_IsHttpsRequired_C("http://mybank.com"));
    
    TEST_ASSERT_FALSE(Http_IsHttpsRequired_C("http://example.com"));
    TEST_ASSERT_FALSE(Http_IsHttpsRequired_C("http://blog.example.com"));
    TEST_ASSERT_FALSE(Http_IsHttpsRequired_C("http://news.example.com"));
    
    TEST_ASSERT_FALSE(Http_IsHttpsRequired_C(nullptr));
}

// Tests d'extraction de domaine
void test_domain_extraction() {
    char domain[64];
    
    // URL HTTP simple
    TEST_ASSERT_TRUE(Http_ExtractDomain_C("http://example.com", domain, sizeof(domain)));
    TEST_ASSERT_EQUAL_STRING("example.com", domain);
    
    // URL HTTPS avec chemin
    TEST_ASSERT_TRUE(Http_ExtractDomain_C("https://api.example.com/v1/users", domain, sizeof(domain)));
    TEST_ASSERT_EQUAL_STRING("api.example.com", domain);
    
    // URL avec port
    TEST_ASSERT_TRUE(Http_ExtractDomain_C("http://localhost:8080/api", domain, sizeof(domain)));
    TEST_ASSERT_EQUAL_STRING("localhost:8080", domain);
    
    // URL sans protocole
    TEST_ASSERT_TRUE(Http_ExtractDomain_C("example.com/path", domain, sizeof(domain)));
    TEST_ASSERT_EQUAL_STRING("example.com", domain);
    
    // URL simple sans chemin
    TEST_ASSERT_TRUE(Http_ExtractDomain_C("https://google.com", domain, sizeof(domain)));
    TEST_ASSERT_EQUAL_STRING("google.com", domain);
    
    // Cas d'erreur
    TEST_ASSERT_FALSE(Http_ExtractDomain_C(nullptr, domain, sizeof(domain)));
    TEST_ASSERT_FALSE(Http_ExtractDomain_C("http://example.com", nullptr, sizeof(domain)));
    TEST_ASSERT_FALSE(Http_ExtractDomain_C("http://example.com", domain, 5)); // Buffer trop petit
}

int main() {
    UNITY_BEGIN();
    
    // Tests construction requête
    RUN_TEST(test_build_request_get);
    RUN_TEST(test_build_request_post_json);
    RUN_TEST(test_build_request_post_form);
    RUN_TEST(test_build_request_invalid);
    
    // Tests validation
    RUN_TEST(test_timeout_validation);
    RUN_TEST(test_content_type_validation);
    RUN_TEST(test_content_type_strings);
    
    // Tests utilitaires
    RUN_TEST(test_https_required_detection);
    RUN_TEST(test_domain_extraction);
    
    return UNITY_END();
}
