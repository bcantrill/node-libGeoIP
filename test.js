assert = require('assert');
GeoIP = require('libGeoIP').libGeoIP;
sys = require('sys');
fs = require('fs');

var shouldthrow = function (func, str)
{
	var fail = undefined;

	try {
		func();
	} catch (err) {
		var msg = sys.inspect(err.toString());

		sys.puts('call failed as expected; message: ' + msg);

		if (msg.indexOf(str) != -1)
			return;

		assert.ok(false, 'failure msg (' + msg + ') did not contain ' +
		    'expected substring (' + sys.inspect(str) + ')');
	}

	assert.ok(false, 'expected failure to contain "' + str +
	    '"; call succeeded!');
}

var find = function (file, fallback)
{
	var dirs = [ '/usr', '/usr/local', '/opt',
	    '/opt/local', process.cwd() ];
	var dir = 'share/GeoIP';

	for (i = 0; i < dirs.length; i++) {
		path = dirs[i] + '/' + dir + '/' + file;

		try {
			fs.statSync(path);
			return (path);
		} catch (err) {
			continue;
		}
	}

	if (!fallback)
		assert.ok(false, 'could not find file \'' + file + '\'');

	return (fallback);
}

shouldthrow(function () { new GeoIP('/does/not/exist') }, 'Error Opening');

geoip = new GeoIP(find('GeoIP.dat'));
shouldthrow(function () { geoip.query('8.12.47.107') }, 'Invalid database');

geoip = new GeoIP(find('GeoLiteCity.dat',
    process.argv.length > 2 ? process.argv[2] : undefined));

var queries = [
	{ fail: 'Doogle.86.75.30.9' },
	{ fail: '10.99.99.5' },
	'86.75.30.9',
	'86.75.3.09',
	'8.67.53.09',
	'8.12.47.107',
	'216.236.135.152',
	'216.57.203.66'
];

var expected = [ 'longitude', 'latitude' ];

for (i = 0; i < queries.length; i++) {
	var ip = (queries[i].fail ? queries[i].fail : queries[i]);
	var result = geoip.query(ip);
	sys.puts('Query for ' + ip + ' is: ' + sys.inspect(result));

	if (queries[i].fail) {
		assert.equal(result, undefined,
		    'did not expect query to succeed!');
	} else {
		for (j = 0; j < expected.length; j++) {
			assert.ok(result.hasOwnProperty(expected[j]),
			    'missing expected property \'' +
			    expected[j] + '\'');
		}
	}
}

