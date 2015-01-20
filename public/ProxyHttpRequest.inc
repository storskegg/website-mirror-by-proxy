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
                'timeout' => Conf::$proxy_http_request_timeout,
                'connecttimeout' => Conf::$proxy_http_request_connecttimeout
            ]);
    }

    public function getUrl()
    {
        static $url;
        if (! isset($url)) {
            $url = RedirectWhenBlockedFull::getRequestUriWithoutQueryStringParam();
            $this->removeThisScriptDirFromUrl($url);
            
            if (RedirectWhenBlockedFull::startsWith($url, '/http://') ||
                 RedirectWhenBlockedFull::startsWith($url, '/https://')) {
                $url = substr($url, 1);
                
                if (! TextExternalUrlFilters::matchesUrl($url)) {
                    header('HTTP/1.0 403 Forbidden');
                    exit();
                }
                
                // Use in DomUtlFilters for relative URLs.
                $base_url_suffix = rtrim(http_build_scheme_host($url), '/') . '/';
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
