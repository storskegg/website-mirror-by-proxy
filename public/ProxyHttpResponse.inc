<?php

class ProxyHttpResponse
{

    const CONTENT_TYPE_TEXT_HTML = 1;

    const CONTENT_TYPE_TEXT_CSS = 2;

    const CONTENT_TYPE_JAVASCRIPT = 3;

    const CONTENT_TYPE_JSON = 4;

    private $body, $headers, $response_code;

    private $request;

    public function __construct(http\Client\Response $response, 
        ProxyHttpRequest $request)
    {
        $this->body = $response->getBody()->toString();
        $this->headers = $response->getHeaders();
        $this->response_code = $response->getResponseCode();
        
        $this->request = $request;
    }

    public function getBody()
    {
        $body = $this->body;
        $content_type = $this->getContentType();
        
        switch ($content_type) {
            
            case self::CONTENT_TYPE_JAVASCRIPT:
            case self::CONTENT_TYPE_JSON:
            case self::CONTENT_TYPE_TEXT_CSS:
            case self::CONTENT_TYPE_TEXT_HTML:
                $body = $this->getBodyFilteredByContentType($body, 
                    $content_type);
                
                // These filters apply to all of the above.
                TextInternalUrlFilters::applyAll($body);
                TextExternalUrlFilters::applyAll($body, $content_type);
                
                // Remove content-length, since this might be different after modification.
                if (isset($this->headers['Content-Length'])) {
                    unset($this->headers['Content-Length']);
                }
                
                Log::add($body);
                break;
        }
        
        return $body;
    }

    public function getHeader($name)
    {
        if (isset($this->headers[$name])) {
            return $this->headers[$name];
        }
    }

    public function getHeaders()
    {
        $headers = $this->headers;
        Log::add($headers, 'headers raw');
        
        // If redirect, rewrite Location header.
        if (isset($headers['Location'])) {
            if (parse_url($headers['Location'], PHP_URL_HOST)) {
                TextExternalUrlFilters::applyAll($headers['Location']);
            }
            
            // Header redirects require full URLs, with scheme and host.
            if (! parse_url($headers['Location'], PHP_URL_HOST)) {
                $headers['Location'] = RedirectWhenBlockedFull::getBaseUrl() .
                     ltrim($headers['Location'], '/');
            }
        }
        
        // Rewrite set-cookie headers.
        if (isset($headers['Set-Cookie'])) {
            if (is_array($headers['Set-Cookie'])) {
                foreach ($headers['Set-Cookie'] as &$set_cookie) {
                    $set_cookie = $this->getFilteredSetCookie($set_cookie);
                }
            } else {
                $headers['Set-Cookie'] = $this->getFilteredSetCookie(
                    $headers['Set-Cookie']);
            }
        }
        
        // Unset some.
        $skip = array(
            'Connection',
            'Transfer-Encoding'
        );
        foreach ($skip as $s) {
            if (isset($headers[$s])) {
                unset($headers[$s]);
            }
        }
        
        Log::add($headers, 'headers filtered');
        
        return $headers;
    }

    public function getResponseCode()
    {
        return $this->response_code;
    }

    private function destroyDom(&$dom)
    {
        $dom->clear();
        unset($dom);
    }

    private function getBodyFilteredByContentType($body, $content_type)
    {
        switch ($content_type) {
            
            case self::CONTENT_TYPE_TEXT_CSS:
                
                $url_path_depth = count(
                    explode('/', 
                        trim($this->request->getUrlComponent('path'), '/'))) - 1;
                if ($url_path_depth) {
                    $css_base_url = str_repeat('../', $url_path_depth);
                } else {
                    $css_base_url = './';
                }
                $body = str_replace('url("../', 'url("' . $css_base_url . '../', $body);
                $body = str_replace('url(\'../', 'url(\'' . $css_base_url . '../', $body);
                $body = str_replace('url(../', 'url(' . $css_base_url . '../', $body);
                $body = str_replace('url("/', 'url("' . $css_base_url, $body);
                $body = str_replace('url(\'/', 'url(\'' . $css_base_url, $body);
                $body = str_replace('url(/', 'url(' . $css_base_url, $body);
                break;
            
            case self::CONTENT_TYPE_TEXT_HTML:
                
                $dom = $this->getDomFromHtml($body);
                
                if ($dom) {
                    
                    foreach ($dom->find('head title') as $title_element) {
                        $title = mb_convert_encoding_plus(
                            $title_element->text(), 'UTF-8', 
                            $this->getCharsetFromHeadersOrDom($dom));
                        if ($title) {
                            $title .= ' | 免翻墙镜像';
                            
                            // Update title in DOM.
                            $title_element->innertext = mb_convert_encoding_plus(
                                $title, $this->getCharsetFromHeadersOrDom($dom), 
                                'UTF-8');
                            break;
                        }
                    }
                }
                
                // Default title.
                if (! isset($title) || ! $title) {
                    $title = '免翻墙镜像';
                }
                
                // Only apply RWB if response is 200 and request method is GET.
                if ($this->getResponseCode() == 200 && $_SERVER['REQUEST_METHOD'] == 'GET') {
                    
                    RedirectWhenBlockedFull::setWebsiteTitle($title);
                    RedirectWhenBlockedFull::run();
                    
                    // RWB will turn output into JSONP. Remove content-type header to keep header sent by RWB.
                    $headers = $this->getHeaders();
                    if (isset($this->headers['Content-Type'])) {
                        unset($this->headers['Content-Type']);
                    }
                } elseif($dom) {
                    
                    $html = $dom->__toString();
                    RedirectWhenBlockedFull::injectBaseTag($html);
                    $this->destroyDom($dom);
                    $dom = $this->getDomFromHtml($html);
                }
                
                // Might be for example JSONP data served with an incorrect header.
                if (! $dom || ! $dom->find('body')) {
                    break;
                }
                
                DomUrlFilters::applyAll($dom, $this->request);

                // Special for iframes. Prevent RWB from hijacking content (iframe in iframe problem).
                foreach ($dom->find('iframe') as $iframe) {
                    $src = $iframe->getAttribute('src');
                    if ($src) {
                        $src = http_add_query_component($src, 
                            RedirectWhenBlockedFull::QUERY_STRING_PARAM_NAME, 
                            RedirectWhenBlockedFull::OUTPUT_TYPE_IFRAME);
                        $iframe->setAttribute('src', $src);
                    }
                }
                
                $dom_do_reset = false;
                
                // Don't insert stuff if request failed.
                if ($this->getResponseCode() == 200) {
                    if (Conf::$html_body_appendix) {
                        foreach ($dom->find('body') as $b) {
                            $b->innertext .= Conf::$html_body_appendix;
                            break;
                        }
                        
                        $dom_do_reset = true;
                    }
                    
                    if (Conf::$html_head_appendix) {
                        foreach ($dom->find('head') as $h) {
                            $h->innertext .= Conf::$html_head_appendix;
                            break;
                        }
                        
                        $dom_do_reset = true;
                    }
                }
                
                if (count($dom->find('script[async]'))) {
                    
                    if ($dom_do_reset) {
                        
                        // Bug in Simple HTML Dom means that DOM has to be reloaded before further edits.
                        $this->resetDom($dom);
                        $dom_do_reset = false;
                    }
                    
                    // Remove 'async' attributes because they break the page in IE11 when loaded in our iframe.
                    foreach ($dom->find('script[async]') as $x) {
                        $x->removeAttribute('async', '');
                    }
                }
                
                if (count($dom->find('style'))) {
                    
                    if ($dom_do_reset) {
                        
                        // Bug in Simple HTML Dom means that DOM has to be reloaded before further edits.
                        $this->resetDom($dom);
                        $dom_do_reset = false;
                    }
                    
                    // Remove 'async' attributes because they break the page in IE11 when loaded in our iframe.
                    foreach ($dom->find('style') as $style) {
                        $style->innertext = $this->getBodyFilteredByContentType(
                            $style->innertext, self::CONTENT_TYPE_TEXT_CSS);
                    }
                }
                
                $body = $dom->__toString();
                $this->destroyDom($dom);
                break;
        }
        
        return $body;
    }

    private function getCharsetFromHeadersOrDom(simple_html_dom $dom)
    {
        // Allow manually overriding this.
        if(Conf::$default_upstream_charset) {
            return Conf::$default_upstream_charset;
        }
        
        // TODO: get charset from headers.
        foreach ($dom->find('meta[http-equiv=Content-Type]') as $meta) {
            $content = $meta->getAttribute('content');
            if ($content) {
                $content_chunks = explode(';', $content, 2);
                if (isset($content_chunks[1])) {
                    $charset_key_value_str = trim($content_chunks[1]);
                    $charset_key_value_arr = explode('=', 
                        $charset_key_value_str, 2);
                    if (isset($charset_key_value_arr[1])) {
                        return $charset_key_value_arr[1];
                    }
                }
            }
        }
    }

    private function getContentType()
    {
        $content_type = $this->getHeader('Content-Type');
        $content_type_chunks = explode(';', $content_type, 2);
        if (count($content_type_chunks) > 1) {
            $content_type = $content_type_chunks[0];
        }
        $content_type = strtolower($content_type);
        
        switch ($content_type) {
            
            case 'application/json':
                return self::CONTENT_TYPE_JSON;
            
            case 'application/javascript':
            case 'application/x-javascript':
            case 'text/javascript':
                return self::CONTENT_TYPE_JAVASCRIPT;
            
            case 'text/css':
                return self::CONTENT_TYPE_TEXT_CSS;
            
            case 'text/html':
                return self::CONTENT_TYPE_TEXT_HTML;
        }
    }

    private function getDomFromHtml($html)
    {
        return str_get_html($html, false, false, '', false);
    }

    private function getFilteredSetCookie($set_cookie)
    {
        $all_chunks = explode(';', $set_cookie);
        $filtered_chunks = array();
        foreach ($all_chunks as &$chunk) {
            $chunk = trim($chunk);
            list ($key) = explode('=', $chunk);
            switch ($key) {
                case 'path':
                case 'domain':
                    continue 2;
                default:
                    $filtered_chunks[] = $chunk;
            }
        }
        return implode('; ', $filtered_chunks);
    }

    private function resetDom(&$dom)
    {
        $html = $dom->__toString();
        $this->destroyDom($dom);
        $dom = $this->getDomFromHtml($html);
    }
}
