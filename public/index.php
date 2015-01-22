<?php
require 'main.inc';

// Default cache.
// Will be overwritten by message below if it has it's own Cache-Control header.
// Send this early, to prevent caching error pages for longer than the duration.
header('Cache-Control: max-age=' . Conf::$default_cache_control_max_age);

Log::add($_SERVER, '$_SERVER');
Log::add(new Conf(), 'Conf');

if (isset($_GET[RedirectWhenBlockedFull::QUERY_STRING_PARAM_NAME]) && $_GET[RedirectWhenBlockedFull::QUERY_STRING_PARAM_NAME] ==
     Conf::OUTPUT_TYPE_ALT_BASE_URLS) {
    
    // Key cannot be empty.
    if (Conf::$alt_base_urls_key) {
        
        // Verify key. Set this in conf-local.inc.
        if (isset($_GET['key']) && $_GET['key'] == Conf::$alt_base_urls_key) {
            
            header('Content-Type: application/javascript');
            print json_encode(RedirectWhenBlockedFull::getAltBaseUrls());
            exit();
        }
    }
}

$request = new ProxyHttpRequest();

// Hijack crossdomain.xml.
if ($request->getUrlComponent('path') == '/crossdomain.xml' &&
     getDownstreamOrigin()) {
    header('Content-Type: application/xml');
    $downstream_origin = getDownstreamOrigin();
    print 
        <<<EOF
<?xml version="1.0" ?>
<cross-domain-policy>
  <site-control permitted-cross-domain-policies="master-only"/>
  <allow-access-from domain="$downstream_origin"/>
  <allow-http-request-headers-from domain="$downstream_origin" headers="*"/>
</cross-domain-policy>
EOF;
    exit();
}

$client = new http\Client();
$client->setOptions(
    [
        'connecttimeout' => Conf::$proxy_http_request_connecttimeout,
        'dns_cache_timeout' => Conf::$proxy_http_request_dns_cache_timeout,
        'retrycount' => Conf::$proxy_http_request_retrycount,
        'timeout' => Conf::$proxy_http_request_timeout
    ]);
$client->enqueue($request)->send();
$response = new ProxyHttpResponse($client->getResponse(), $request);

$body = $response->getBody();
$headers = $response->getHeaders();

if (getDownstreamOrigin()) {
    $headers['Access-Control-Allow-Origin'] = getDownstreamOrigin();
    
    // See http://stackoverflow.com/questions/12409600/error-request-header-field-content-type-is-not-allowed-by-access-control-allow.
    $headers['Access-Control-Allow-Headers'] = 'Origin, X-Requested-With, Content-Type, Accept';
}

foreach ($headers as $key => $values) {
    if (! is_array(($values))) {
        $values = array(
            $values
        );
    }
    
    foreach ($values as $i => $value) {
        header($key . ': ' . $value, ($i == 0));
    }
}

print $body;