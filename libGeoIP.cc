#include <v8.h>
#include <node.h>
#include <string.h>
#include <unistd.h>
#include <node_object_wrap.h>
#include <errno.h>
#include <stdarg.h>
#include <strings.h>
#include <fcntl.h>
#include "GeoIP.h"
#include "GeoIPCity.h"

using namespace v8;
using namespace node;

/*
 * Wrap ourselves in a namespace to allow us to re-use GeoIP (a typedef from
 * the GeoIP header files) as our class name.
 */
namespace {

class GeoIP : node::ObjectWrap {
public:
	static void Initialize(Handle<Object> target);

protected:
	GeoIP(const char *, int);
	~GeoIP();

	static Handle<Value> error(const char *fmt, ...);
	static Handle<Value> badarg(const char *);
	static Handle<Value> New(const Arguments& args);
	static Handle<Value> Query(const Arguments& args);

private:
	::GeoIP *geoip_handle;
	int geoip_errfd;
};

Handle<Value>
GeoIP::error(const char *fmt, ...)
{
	char buf[1024], buf2[1024];
	char *err = buf;
	va_list ap;

	va_start(ap, fmt);
	(void) vsnprintf(buf, sizeof (buf), fmt, ap);

	if (buf[strlen(buf) - 1] != '\n') {
		/*
		 * If our error doesn't end in a new-line, we'll append the
		 * strerror of errno.
		 */
		(void) snprintf(err = buf2, sizeof (buf2),
		    "%s: %s", buf, strerror(errno));
	} else {
		buf[strlen(buf) - 1] = '\0';
	}

	return (Exception::Error(String::New(err)));
}

Handle<Value>
GeoIP::badarg(const char *msg)
{
        return (ThrowException(Exception::TypeError(String::New(msg))));
}

GeoIP::GeoIP(const char *file, int flags = GEOIP_STANDARD) : node::ObjectWrap()
{
	FILE err = *stderr;
	char buf[1024];
	int saved = -1, fd;
	char *tmpfile;

	/*
	 * GeoIP_open() has the rather annoying habit of writing failure
	 * messages to stderr to denote failure -- an annoyance bested only
	 * by the lookup routines' use of stdout to denote same!  We therefore
	 * create a temporary file that we will use to capture detailed error
	 * information by (temporarily) dup'ing to the underlying file
	 * stderr/stdout file descriptors.
	 */
	if ((tmpfile = tmpnam(NULL)) == NULL)
		throw (error("couldn't get temporary name"));

	if ((fd = open(tmpfile, O_CREAT | O_RDWR | O_EXCL | O_TRUNC, 0)) == -1)
		throw (error("couldn't create %s", file));

	if (unlink(tmpfile) != 0) {
		(void) close(fd);
		throw (error("couldn't unlink %s", file));
	}

	fflush(stderr);
	saved = dup(STDERR_FILENO);
	
	if ((saved = dup(STDERR_FILENO)) < 0 || dup2(fd, STDERR_FILENO) < 0) {
		(void) close(fd);
		(void) close(saved);
		throw (error("couldn't dup fd %d to stderr", fd));
	}

	geoip_handle = GeoIP_open(file, flags);

	fflush(stderr);
	(void) dup2(saved, STDERR_FILENO);
	(void) close(saved);
	geoip_errfd = fd;

	if (geoip_handle != NULL)
		return;

	bzero(buf, sizeof (buf));
	(void) pread(fd, buf, sizeof (buf) - 1, 0);

	if (buf[0] == '\0') {
		(void) snprintf(buf, sizeof (buf),
		    "GeoIP_open error: %s", strerror(saved));
	}

	throw (Exception::Error(String::New(buf)));
};

GeoIP::~GeoIP()
{
	GeoIP_delete(geoip_handle);
	(void) close(geoip_errfd);
}

void
GeoIP::Initialize(Handle<Object> target)
{
	HandleScope scope;
	Local<FunctionTemplate> t = FunctionTemplate::New(GeoIP::New);
	t->InstanceTemplate()->SetInternalFieldCount(1);

	NODE_SET_PROTOTYPE_METHOD(t, "query", GeoIP::Query);

	target->Set(String::NewSymbol("libGeoIP"), t->GetFunction());
}

Handle<Value>
GeoIP::New(const Arguments& args)
{
	HandleScope scope;
	GeoIP *geoip;

	String::AsciiValue file(args[0]->ToString());

	try {
		geoip = new GeoIP(*file);
	} catch (Handle<Value> exception) {
		return (ThrowException(exception));
	}

	geoip->Wrap(args.Holder());

	return (args.This());
}

Handle<Value>
GeoIP::Query(const Arguments& args)
{
	HandleScope scope;
	GeoIP *geoip = ObjectWrap::Unwrap<GeoIP>(args.Holder());
	::GeoIP *gi = geoip->geoip_handle;
	GeoIPRecord *rec;	
	int err = -1, out = -1, fd = geoip->geoip_errfd;

	struct {
		const char *name;
		int offset;
	} *field, floats[] = {
		{ "latitude", offsetof (GeoIPRecord, latitude) },
		{ "longitude", offsetof (GeoIPRecord, longitude) },
		{ NULL }
	}, ints[] = {
		{ "metro_code", offsetof (GeoIPRecord, metro_code) },
		{ "area_code", offsetof (GeoIPRecord, area_code) },
		{ NULL }
	}, strings[] = {
		{ "country_code", offsetof (GeoIPRecord, country_code) },
		{ "country_code3", offsetof (GeoIPRecord, country_code3) },
		{ "continent_code", offsetof (GeoIPRecord, continent_code) },
		{ "country", offsetof (GeoIPRecord, country_name) },
		{ "region", offsetof (GeoIPRecord, region) },
		{ "city", offsetof (GeoIPRecord, city) },
		{ "postal_code", offsetof (GeoIPRecord, postal_code) },
		{ NULL }
	}; 

	if (args.Length() < 1 || !args[0]->IsString()) {
		return (badarg("expected a string representing an "
		    "IP address to geolocate"));
	}

	String::AsciiValue addr(args[0]->ToString());

	/*
	 * As comments above and below indicate, libGeoIP expresses failure
	 * by writing to stderr/stdout.  To capture these failures, we need
	 * to interpose on this behavior -- which is most cleanly done by
	 * interposing on the underlying file descriptors.
	 */
	fflush(stderr);
	fflush(stdout);

	if ((out = dup(STDOUT_FILENO)) < 0 || dup2(fd, STDOUT_FILENO) < 0 ||
	    (err = dup(STDERR_FILENO)) < 0 || dup2(fd, STDERR_FILENO) < 0) {
		Handle<Value> xcp = error("couldn't move aside stdout/stderr");
		(void) dup2(out, STDOUT_FILENO);
		(void) dup2(err, STDERR_FILENO);
		(void) close(out);
		(void) close(err);
		return (ThrowException(xcp));
	}

	rec = GeoIP_record_by_addr(gi, *addr);

	fflush(stderr);
	fflush(stdout);
	(void) dup2(out, STDOUT_FILENO);
	(void) dup2(err, STDERR_FILENO);
	(void) close(out);
	(void) close(err);

	if (rec == NULL) {
		char buf[1024];
		ssize_t nbytes;

		/*
		 * If we've been returned a NULL record, we sadly don't know
		 * if this was due to an error or simply missing geo data for
		 * the IP address.  To differentiate these cases, we determine
		 * if anything has been written to stderr or (amazingly)
		 * stdout -- treating this as the detailed information on
		 * failure where it is non-zero.
		 */
		if ((nbytes = pread(fd, buf, sizeof (buf) - 1, 0)) == 0)
			return (Undefined());

		buf[nbytes] = '\0';

		return (ThrowException(error(buf)));
	}

	Handle<Object> rval = Object::New();

	for (field = floats; field->name != NULL; field++) {
		rval->Set(String::New(field->name),
		    Number::New(*((float *)((uintptr_t)rec + field->offset))));
	}

	for (field = strings; field->name != NULL; field++) {
		char *val = *((char **)((uintptr_t)rec + field->offset));

		if (val == NULL)
			continue;

		rval->Set(String::New(field->name), String::New(val));
	}

	for (field = ints; field->name != NULL; field++) {
		int val = *((int *)((uintptr_t)rec + field->offset));

		if (val == 0)
			continue;

		rval->Set(String::New(field->name), Number::New(val));
	}

	return (rval);
}

extern "C" void
init (Handle<Object> target) 
{
	GeoIP::Initialize(target);
}

}
