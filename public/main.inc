<?php
require 'Conf.inc';
require 'filters/DomUrlFilters.inc';
require 'filters/TextExternalUrlFilters.inc';
require 'filters/TextInternalUrlFilters.inc';
require 'Log.inc';
require 'ProxyHttpRequest.inc';
require 'ProxyHttpResponse.inc';
require 'simple_html_dom.php';
require 'url.inc';
require 'rwb/RedirectWhenBlockedFull.inc';

require 'conf-local.inc';

function getDownstreamOrigin()
{
	static $downstream_origin_verified;
	if (! isset($downstream_origin_verified)) {
		$downstream_origin_verified = NULL;
		
		if (isset($_SERVER['HTTP_ORIGIN'])) {
			$downstream_origin = $_SERVER['HTTP_ORIGIN'];
		} elseif (isset($_SERVER['HTTP_REFERER'])) {
			$downstream_origin = http_build_scheme_host(
				$_SERVER['HTTP_REFERER']);
		}
		
		if (isset($downstream_origin)) {
			foreach (RedirectWhenBlockedFull::getAltBaseUrls() as $alt_url_base) {
				if ($downstream_origin == http_build_scheme_host($alt_url_base)) {
					$downstream_origin_verified = $downstream_origin;
					break;
				}
			}
		}
	}
	
	return $downstream_origin_verified;
}

RedirectWhenBlockedFull::setUrlsFromConfDir();

TextExternalUrlFilters::addHost(
	Conf::getDefaultUpstreamBaseUrlComponent('host'));

DomUrlFilters::addAttribute('action');
DomUrlFilters::addAttribute('href');
DomUrlFilters::addAttribute('src');