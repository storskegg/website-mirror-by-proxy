<?php

class AltBaseUrlsTest extends StatusTest
{

    function run()
    {
        if (count(RedirectWhenBlockedFull::getAltBaseUrls()) == 0) {
            $this->messages[] = 'Empty alt base urls';
            return;
        }
        
        foreach (RedirectWhenBlockedFull::getAltBaseUrls() as $url) {
            $ch = curl_init($url);
            curl_setopt($ch, CURLOPT_RETURNTRANSFER, 1);
            $result = curl_exec($ch);
            $http_status = curl_getinfo($ch, CURLINFO_HTTP_CODE);
            curl_close($ch);
            if ($http_status == 200) {
                $this->messages[] = "$url passed ($http_status)";
            } else {
                $this->n_failed ++;
                $this->messages[] = "$url failed ($http_status)";
            }
        }
        
        $alt_base_urls_file = dirname(dirname(__DIR__)) .
             '/rwb/conf/alt_base_urls.txt';
        $alt_base_urls_modified_ago = time() - filemtime($alt_base_urls_file);
        $this->messages[] = "$alt_base_urls_file modified $alt_base_urls_modified_ago seconds ago";
        if ($alt_base_urls_modified_ago > 60 * 60 * 24) {
            $this->messages[] = "$alt_base_urls_file expired";
            return false;
        }
        
        return true;
    }
}