assert = require('assert');
GeoIP = require('libGeoIP').libGeoIP;
sys = require('sys');

assert.throws(function () { new GeoIP('/this/db/does/not/exist') });

geoip = new GeoIP('/usr/local/share/GeoIP/GeoIP.dat');

assert.throws(function () { geoip.query('8.12.47.107') } );

geoip = new GeoIP('/usr/local/share/GeoIP/GeoLiteCity.dat');

sys.puts(sys.inspect(geoip.query('Doogle.86.75.30.9')));
sys.puts(sys.inspect(geoip.query('86.75.30.9')));
sys.puts(sys.inspect(geoip.query('8.12.47.107')));
sys.puts(sys.inspect(geoip.query('10.99.99.5')));
sys.puts(sys.inspect(geoip.query('216.236.135.152')));
sys.puts(sys.inspect(geoip.query('216.57.203.66')));

