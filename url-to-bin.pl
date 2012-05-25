#!/usr/bin/perl

use MIME::Base64;

($data) = ($ARGV[0] =~ /\?doodle=6201726X(.+?)(&|$)/);
$data =~ tr/ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789\-_\./ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789\+\/=/;
print decode_base64($data);

