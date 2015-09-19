#!/usr/bin/perl

use strict;
use warnings;

use CGI qw/:standard/;
use Template;
use Mail;
use DBI;
use Crypt::PasswdMD5 qw(unix_md5_crypt);

require './config.pl';


sub cgi_new {
if (($ENV{REQUEST_METHOD}||"") eq "POST") {
	my $post = $ENV{REQUEST_BODY_1};
	if (!defined $post) { $post = ""; }
	my $len = $ENV{CONTENT_LENGTH};
	if (defined $len && ($len -= length($post)) >= 0) {
		read(STDIN, $post, $len, length($post));
	} else {
		$post .= join "", <STDIN>;
	}
	return CGI->new($post);
} else {
	return CGI->new();
}
}


my $cgi = cgi_new();

my $name = $cgi->param("name");
my $house = $cgi->param("house");
my $street = $cgi->param("street");
my $city = $cgi->param("city");
my $region = $cgi->param("region");
my $country = $cgi->param("country"); 
my $email = $cgi->param("email");
my $phone = $cgi->param("phone");
#XXX MARK
my $username = $cgi->param("username");
my $password = $cgi->param("password");
my $confirm = $cgi->param("confirm");
my $zip = $cgi->param("postcode");

if ($username eq "" || $confirm eq "") {
	browser_message("Sorry, there are missing or empty fields in the form.");
}

if ($name eq ""  || $email eq "" ) {
	browser_message("Sorry, there are missing or empty fields in the form.");
} 

	
$name =~ s{(['%])}{\\$1}g;
$username =~ s{(['%])}{\\$1}g;
$password =~ s{(['%])}{\\$1}g;
$confirm =~ s{(['%])}{\\$1}g;
$email =~ s{(['%])}{\\$1}g;

if ($password ne $confirm) {
	browser_message("The passwords you entered did not match, please try again");
}

if ($username =~ /[A-Z]+/)
{
	browser_message("Username cannot have upper-case letters!");
}

if (uc($email) !~ /^[A-Z0-9._%+-]+@[A-Z0-9.-]+\.[A-Z]{2,4}$/) {
	browser_message("Invalid e-mail address, try again!");
}

my $data = {
	"NAME"    => $name,
	"HOUSE"   => $house,
	"STREET"  => $street,
	"CITY"    => $city,
	"REGION"  => $region,
	"COUNTRY" => $country,
	"EMAIL"   => $email,
	"PHONE"   => $phone,
	"USERNAME" => $username,
	"PASSWORD" => $password,
};

my $template = Template->new();
my $body = "";
$template->process("templates/email", $data, \$body);

my $sendmail = Mail->Sendmail();

$sendmail->Send('to' => 'netstar@gmail.com',
		'from' => $email,
		'subject' => 'Sanctoshare: REGISTRATION',
		'body' => $body);

my $dbh = dbh_connect();

my $SQL =  "";
my $sth =  "";


$SQL = "SELECT * FROM users WHERE username = '$username'";
$sth = $dbh->prepare($SQL);
$sth->execute() || die "First usercheck\n\n";

my $exists = $sth->fetchrow();
if ($exists ne "") {
	browser_message("Sorry, that username already exists, please choose another.");
}

$SQL = "INSERT INTO projects (username, confirm, name, house, phone, street, city, email, password, country, region, zip) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

$sth = $dbh->prepare($SQL);
$sth->execute($username, $confirm, $name, $house, $phone, $street, $city, $email, $password, $country, $region, $zip) || die "sth->execute - Initial insert\n\n";

$SQL = "SELECT id FROM projects WHERE username = '$username'";
$sth = $dbh->prepare($SQL);
$sth->execute() || die "sth->execute - Second insert\n\n";

my $id = $sth->fetchrow();

my $key = md5_hex(time());
$SQL = "INSERT INTO users (code, active, username, password, userid) VALUES (?, ?, ? , ?, ?)";
$sth = $dbh->prepare($SQL);
$sth->execute($key, 0, $username, $password, $id) || die "sth->execute third insert\n\n";



my $confirmation = Template->new();
$body = "";
my $vars = {
	"NAME" => $name,
	"USERNAME" => $username,
	"PASSWORD" => $password,
	"KEY" => $key,

};

$confirmation->process("templates/confirmation", $vars, \$body);

my $mail = Mail->Sendmail();
$mail->Send( to => $email,
	     from => 'noreply@haxlab.org',
	     subject => 'Haxlab Hosting',
	     body => $body);

my $md5_pwd = unix_md5_crypt($password, gensalt(8));

#my $USER_ADD = "sshc root\@bryanpoole.demon.co.uk /usr/sbin/useradd $username -m -d /home/$username -p \'$md5_pwd\'";


	browser_message("You have succesfully registered with us, to complete your registration follow the instructions on the e-mail sent to your inbox.");
exit 0;
