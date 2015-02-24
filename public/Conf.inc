<?php

class Conf
{
    
    // Using the same QUERY_STRING_PARAM_NAME as in RWB so make sure this value isn't used by RWB.
    const OUTPUT_TYPE_ALT_BASE_URLS = 10;

    const OUTPUT_TYPE_APK_URLS = 11;

    const OUTPUT_TYPE_STATUS = 12;

    const OUTPUT_TYPE_APK = 'apk';

    public static $alt_base_urls_key;
    
    // 1 day.
    public static $alt_base_urls_modified_ago_max = 86400;

    public static $apk_url;

    public static $cookies_enabled = false;

    public static $default_cache_control_max_age = 60;
    
    // 1 day.
    public static $default_cache_control_stale_if_error = 86400;

    public static $default_cache_control_stale_while_revalidate = 600;

    public static $default_upstream_base_url;
    
    // This should only be used if upstream server isn't configured correctly. Will override server responses.
    public static $default_upstream_charset;

    public static $default_upstream_url;

    public static $html_body_appendix = '';

    public static $html_head_appendix = '';

    public static $proxy_http_request_connecttimeout = 10;

    public static $proxy_http_request_dns_cache_timeout = 60;

    public static $proxy_http_request_retrycount = 1;

    public static $proxy_http_request_timeout = 30;

    public static $china_ip_for_dns_poisoning_test;

    public static function appendToHtmlBody($str)
    {
        self::$html_body_appendix .= $str;
    }

    public static function appendToHtmlHead($str)
    {
        self::$html_head_appendix .= $str;
    }

    public static function getDefaultUpstreamBaseUrlComponent($name)
    {
        $components = self::getDefaultUpstreamBaseUrlComponents();
        if (isset($components[$name])) {
            return $components[$name];
        }
    }

    public static function getDefaultUpstreamBaseUrlComponents()
    {
        static $components;
        if (! isset($components)) {
            $components = parse_url(self::$default_upstream_base_url);
        }
        return $components;
    }
}