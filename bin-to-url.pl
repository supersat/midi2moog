#!/usr/bin/perl

use MIME::Base64;

while (<>) {
	$data .= $_;
}

$data = encode_base64($data);
$data =~ tr/ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789\+\/=/ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789\-_\./;
$data =~ s/\n//g;
print "https://www.google.com/webhp?doodle=6201726X$data\n";

