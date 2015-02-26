<?php

class AltBaseUrlsTest extends StatusTest
{

    function run()
    {
        if (count(RedirectWhenBlockedFull::getAltBaseUrls()) == 0) {
            $this->messages[] = 'Empty alt base urls';
            return;
        }
        
        $domains = array();
        
        foreach (RedirectWhenBlockedFull::getAltBaseUrls() as $url) {
            // Test HTTP response code.
            $ch = curl_init($url);
            curl_setopt($ch, CURLOPT_RETURNTRANSFER, 1);
            curl_setopt($ch, CURLOPT_FOLLOWLOCATION, 1);
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
            $domain = parse_url($url, PHP_URL_HOST);
            $domains[] = $domain;
        }
        
        if (! Conf::$china_ip_for_dns_poisoning_test) {
            $this->messages[] = 'china_ip_for_dns_poisoning_test not set';
            $this->n_failed ++;
        } else {
            $command = '';
            foreach ($domains as $domain) {
                $command .= '(dig +time=5 +tries=1 @' .
                     Conf::$china_ip_for_dns_poisoning_test . ' ' . $domain .
                     ' > /dev/null ; echo "' . $domain . ':"$?)  & ';
            }
            $command = trim($command) . '& wait';
            exec($command, $output);
            $n_domains_not_poisoned = 0;
            foreach ($output as $line) {
                $line_chunks = explode(':', $line, 2);
                if (count($line_chunks) == 2 && $line_chunks[0] &&
                     $line_chunks[1] >= 0) {
                    list ($domain, $dig_exit_code) = $line_chunks;
                    if ($dig_exit_code == 0) {
                        $this->messages[] = $domain . ' is poisoned (' .
                             $dig_exit_code . ')';
                    } else {
                        $this->messages[] = $domain . ' is not poisoned (' .
                             $dig_exit_code . ')';
                        $n_domains_not_poisoned++;
                    }
                } else {
                    $this->messages[] = 'unknown dig result (' . $line . ')';
                }
            }
            
            if(!$n_domains_not_poisoned) {
                $this->messages[] = 'no unpoisoned domains available';
                $this->n_failed ++;
            }
        }
        
        $alt_base_urls_file = dirname(dirname(__DIR__)) .
             '/rwb/conf/alt_base_urls.txt';
        $alt_base_urls_modified_ago = time() - filemtime($alt_base_urls_file);
        $this->messages[] = "$alt_base_urls_file modified $alt_base_urls_modified_ago seconds ago";
        if ($alt_base_urls_modified_ago > Conf::$alt_base_urls_modified_ago_max) {
            $this->messages[] = "$alt_base_urls_file expired";
            return false;
        }
        
        return true;
    }
}