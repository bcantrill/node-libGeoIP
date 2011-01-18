
node-libGeoIP
=============

Overview
--------

node-libGeoIP is a Node.js addon that interfaces to libGeoIP, an LGPL'd
library available from MaxMind that allows programs to perfrom IP-based
geolocation.  This addon seeks to provide a minimal but robust interface to
libGeoIP.  As such, it does not present an asynchronous API (libGeoIP has
none, and is either in-memory or of minimal local I/O anyway) and nor does it
attempt to horn libGeoIP's simple interfaces into anything more complicated.
In short, it is implemented the way you would implement it.

Installation
------------

As an addon, node-libGeoIP is installed in the usual way:

      % node-waf configure
      % node-waf build
      % node-waf

It depends on an installation of `libGeoIP`, available separately.

API
---

### `new libGeoIP(database)`

Create a new libGeoIP consumer.  `database` must be a path to a database.
For the freely available MaxMind database, this is generally of the form:

        /usr/local/share/GeoIP/GeoLiteCity.dat

### `libGeoIP.query(addr)`

Perform a geolocation query for the specified IPv4 address, encoded as a
dot-delimited string.  If the specified address does not match a record,
`undefined` will be returned.  If a record is found, it will be returned
as an object with the following fields:

* `longitude` is the floating-point longitude
* `latitude` is the floating-point latitude

Additionally, the returned object may contain one or more of the following
fields:

* `country_code` is the two letter country code of the matching country
* `country_code3` is the three letter country code of the matching country
* `continent_code` is the two letter continent code
* `country` is the name of the matching country
* `region` is the name of the matching region.  For the US this is the
  state name; for Canada this is the province name; for other countries
  it is the FIPS 10-4 subcountry code.
* `postal_code` is a string containing the postal code (if US or Canada)
* `area_code` is the telephone area code (if US or Canada)
* `metro_code` is the metro-area code (if US)

