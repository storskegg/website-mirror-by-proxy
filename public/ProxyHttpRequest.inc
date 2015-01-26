<?php

class ProxyHttpRequest extends http\Client\Request
{

    public function __construct()
    {
        parent::__construct($_SERVER['REQUEST_METHOD'], $this->getUrl());
        
        if (isset($_POST)) {
            parent::getBody()->addForm($_POST);
        }
        
        $this->setOptions(
            [
                'connecttimeout' => Conf::$proxy_http_request_connecttimeout,
                'dns_cache_timeout' => Conf::$proxy_http_request_dns_cache_timeout,
                'retrycount' => Conf::$proxy_http_request_retrycount,
                'timeout' => Conf::$proxy_http_request_timeout
            ]);
    }

    public function getUrl()
    {
        static $url;
        if (! isset($url)) {
            
            if ($_GET[RedirectWhenBlockedFull::QUERY_STRING_PARAM_NAME] ==
                 Conf::OUTPUT_TYPE_APK && Conf::$apk_url) {
                
                $url = Conf::$apk_url;
                $filename = basename(parse_url($url, PHP_URL_PATH));
                header('Content-Disposition: attachment; filename=' . $filename);
                
                // Run after all other code to override other content-type header.
                register_shutdown_function(
                    function ()
                    {
                        header(
                            'Content-Type: application/vnd.android.package-archive');
                    });
            } else {
                $url = RedirectWhenBlockedFull::getRequestUriWithoutQueryStringParam();
                $this->removeThisScriptDirFromUrl($url);
                
                if (RedirectWhenBlockedFull::startsWith($url, '/http://') ||
                     RedirectWhenBlockedFull::startsWith($url, '/https://')) {
                    $url = substr($url, 1);
                    
                    if (! TextExternalUrlFilters::matchesUrl($url)) {
                        header('HTTP/1.0 403 Forbidden');
                        exit();
                    }
                    
                    // If we for some reason have the default upstream host in the URL, remove it.
                    $url_components = parse_url($url);
                    if ($url_components['host'] ==
                         Conf::getDefaultUpstreamBaseUrlComponent('host')) {
                        $new_url = http_build_path_query_fragment(
                            $url_components);
                        $new_url = RedirectWhenBlockedFull::getBaseUrl() .
                             ltrim($new_url, '/');
                        header('Location: ' . $new_url);
                        exit();
                    }
                    
                    // Use in DomUtlFilters for relative URLs.
                    $base_url_suffix = rtrim(http_build_scheme_host($url), '/') .
                         '/';
                    RedirectWhenBlockedFull::setBaseUrlSuffix($base_url_suffix);
                } else {
                    
                    if ($url == '/') {
                        if (Conf::$default_upstream_url) {
                            $url = Conf::$default_upstream_url;
                        }
                    }
                    $url = Conf::$default_upstream_base_url . $url;
                }
            }
        }
        
        // Reverse rewrites of parameters inside URL.
        TextExternalUrlFilters::applyReverse($url);
        return $url;
    }

    public function getUrlComponent($name)
    {
        $components = $this->getUrlComponents();
        if (isset($components[$name])) {
            return $components[$name];
        }
    }

    public function getUrlComponents()
    {
        static $components;
        if (! isset($components)) {
            $components = parse_url($this->getUrl());
        }
        return $components;
    }

    private static function removeThisScriptDirFromUrl(&$url)
    {
        $this_script_dir = dirname($_SERVER['SCRIPT_NAME']);
        if ($this_script_dir != '/' &&
             substr($url, 0, strlen($this_script_dir)) == $this_script_dir) {
            $url = substr($url, strlen($this_script_dir));
        }
        return $url;
    }
}
