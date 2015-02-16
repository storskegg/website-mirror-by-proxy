<?php

class AltBaseUrlsTest extends StatusTest {
    
    function run() {
        if(count(RedirectWhenBlockedFull::getAltBaseUrls()) == 0) {
            $this->messages[] = 'Empty alt base urls';
            return;
        }
        
        foreach(RedirectWhenBlockedFull::getAltBaseUrls() as $url) {
            $this->messages[] = $url;
        }
        
        $alt_base_urls_file = dirname(dirname(__DIR__)) . '/rwb/conf/alt_base_urls.txt';
        $alt_base_urls_modified_ago = time() - filemtime($alt_base_urls_file);
        $this->messages[] = "$alt_base_urls_file modified $alt_base_urls_modified_ago seconds ago";
        if($alt_base_urls_modified_ago > 60 * 60 * 24) {
            $this->messages[] = "$alt_base_urls_file expired";
            return false;
        }
        
        return true;
    }
}