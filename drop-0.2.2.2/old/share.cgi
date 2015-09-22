#! /usr/bin/perl
#
# This is okay with Apache/Nginx but Node lets us and sets us free!
#

use strict;
use warnings;
# CGI.pm 
use CGI;

sub Error {
	my ($error) = @_;
	print "Content-type: text/plain\r\n\r\n";
	print "status: $error\r\n";

	exit 1 << $error;
}


use DBI;
require '../engine/config.pl';

sub get_file {
	my $data = "";

        if ($ENV{'REQUEST_METHOD'} eq "POST") {
                my $len = $ENV{CONTENT_LENGTH};
		if (defined $len) {
                	read(STDIN, $data, $len);
		}

		my $cgi = CGI->new();

		my $username = $cgi->http('Username');
		my $password = $cgi->http('Password');
		my $filename = $cgi->http('Filename');
		my $action   = $cgi->http('Action');

		$filename =~ s/\.\.//g;
		$filename =~ s/\///g;

		if (!defined $action || ! defined $username || ! defined $password)
		{
			Error(0x0003);
		}

		my $dbh = dbh_connect();
		my $SQL = "SELECT password FROM users WHERE username = ? AND active = 1";
		my $sth = $dbh->prepare($SQL);
		$sth->execute($username);
	
		my $users_password = $sth->fetchrow();
		
		$dbh->disconnect();
	
		if ($username eq "" || $password eq "") {
			Error(0x0005);
		}
	
		if ($password ne $users_password) {
			if ($action eq "AUTH") {
				Error(0x0001);
			} else {
				Error(0x0001);
			}
		}
	
        	if (! -e $username && ! -d $username) {
			mkdir($username, 0755);
        	}

      		our $SLASH = '/';

	        my $path = $username . $SLASH . $filename;
		if ($action eq "ADD") {
			open (FH, "> $path") or die "$!";
			print FH $data;
			close FH;
		} elsif ($action eq "DEL") {
			unlink($path);
		} else {
				
		}

		print "Content-type: text/plain\r\n\r\n";
		print "status: 0\r\n";
	} 
	else 
	{
		browser_message("nope");
	}
}

get_file();

exit 0;
