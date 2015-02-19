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
            // Test HTTP response code.
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
            
            // Test DNS poisoning.
            if (! Conf::$china_ip_for_dns_poisoning_test) {
                $this->messages[] = 'china_ip_for_dns_poisoning_test not set';
                $this->n_failed ++;
            } else {
                $domain = parse_url($url, PHP_URL_HOST);
                $command = 'dig +time=5 +tries=1 @' . Conf::$china_ip_for_dns_poisoning_test . ' ' .
                     $domain;
                exec($command, $output, $return_var);
                if ($return_var == 0) {
                    $this->n_failed ++;
                    $this->messages[] = "$domain poisoned ($return_var)";
                } else {
                    $this->messages[] = "$domain not poisoned ($return_var)";
                }
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