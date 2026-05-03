const char *HTTP_200_OK = 
"HTTP/1.1 200 OK\r\n"
"Access-Control-Allow-Origin: *\r\n"
"Content-Type: text/plain\r\n"
"Content-Length: 2\r\n"
"Connection: close\r\n"
"\r\n"
"OK";

const char *HTTP_404_HEADER =
"HTTP/1.1 404 Not Found\r\n"
"Content-Type: text/html\r\n"
"Content-Length: 126\r\n"
"Connection: close\r\n"
"\r\n";

const char *HTTP_404_HTML =
"<!DOCTYPE html>\n" /* 16 */
"<html>\n" /* 7 */
	"<head>\n" /* 7 */
		"<title>404 Not Found</title>\n" /* 29 */
	"</head>\n" /* 8 */
	"<body>\n" /* 7 */
		"<h1>404 Not Found</h1>\n" /* 23 */
		"No such file.\n" /* 14 */
	"</body>\n" /* 8 */
"</html>"; /* 7 */
/* 126 */

const char *HTTP_500_HTML =
"<!DOCTYPE html>\n"
"<html>\n"
	"<head>\n"
		"<title>500 Internal Server Error</title>\n"
	"</head>\n"
	"<body>\n"
		"<h1>500 Internal Server Error</h1>\n"
		"Something's gone wrong on our end.\n"
	"</body>\n"
"</html>";

const char *HTTP_500_HEADER =
"HTTP/1.1 500 Internal Server Error\r\n"
"Access-Control-Allow-Origin: *\r\n"
"Connection: close\r\n"
"\r\n";


