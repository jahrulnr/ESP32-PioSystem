#ifndef API_CONTROLLER_H
#define API_CONTROLLER_H

#include "../MVCFramework/src/Http/Controller.h"
#include "../HttpClient/src/httpclient.h"
#include <ArduinoJson.h>

/**
 * @brief API Controller for third-party API integration
 * 
 * This controller demonstrates how to use the HttpClient library
 * to fetch data from external APIs and serve it through your web interface.
 */
class ApiController : public Controller {
public:
    ApiController(HttpClientManager* httpClient);
    ~ApiController();

    /**
     * @brief Get weather data for a specific city
     * 
     * @param request HTTP request object
     * @return Response JSON response with weather data
     */
    Response getWeather(Request& request);

    /**
     * @brief Get current time from world time API
     * 
     * @param request HTTP request object  
     * @return Response JSON response with time data
     */
    Response getTime(Request& request);

    /**
     * @brief Test API connectivity
     * 
     * @param request HTTP request object
     * @return Response JSON response with connectivity test results
     */
    Response testConnectivity(Request& request);

    /**
     * @brief Get HTTP client statistics
     * 
     * @param request HTTP request object
     * @return Response JSON response with client statistics
     */
    Response getStats(Request& request);

    /**
     * @brief Proxy request to external API
     * 
     * @param request HTTP request object
     * @return Response Proxied response from external API
     */
    Response proxy(Request& request);

private:
    HttpClientManager* _httpClient;
    
    /**
     * @brief Create error response
     * 
     * @param message Error message
     * @return JsonDocument Error response object
     */
    JsonDocument createErrorResponse(const String& message);
    
    /**
     * @brief Validate required parameters
     * 
     * @param request HTTP request object
     * @param params List of required parameter names
     * @return String Empty if valid, error message if invalid
     */
    String validateParams(Request& request, const std::vector<String>& params);
};

#endif // API_CONTROLLER_H
