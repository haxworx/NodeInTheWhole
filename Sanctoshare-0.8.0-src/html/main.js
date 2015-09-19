function showemail()
{
	document.getElementById("contact").innerHTML = "<b>email</b> netstar@gmail.com";
}

function hideemail()
{
	document.getElementById("contact").innerHTML = '<strong>email</strong> echo -n "bmV0c3RhckBnbWFpbC5jb20K" | base64 --decode';
}
function validateReg()
{
	var missing = 0;
	var name = document.forms["register"]["name"].value;
	if (name == null || name == "") {
		missing = 1;
                document.getElementById("name").style.borderColor = "Red";
	}
	
	var email = document.forms["register"]["email"].value;
        if (email == null || email == "") {
                missing = 1;
                document.getElementById("email").style.borderColor = "Red";

        }

	var username = document.forms["register"]["username"].value;
        if (username == null || username  == "") {
                missing = 1;
                document.getElementById("username").style.borderColor = "Red";
        }

	var password = document.forms["register"]["password"].value;
        if (password == null || password == "") {
		document.getElementById("password").style.borderColor = "Red";
                missing = 1;
        }

	var confirm = document.forms["register"]["confirm"].value;
        if (confirm == null || confirm == "") {
                missing = 1;
		document.getElementById("confirm").style.borderColor = "Red";
        }

	if (missing)
	{
		document.getElementById("formError").innerHTML = "Empty fields!";
		return false;
	}

	return true;
}

function validateLogin()
{
	var missing = 0;
	var u = document.forms["myLogin"]["username"].value;
	if (u == null || u == "") {
		missing = 1;
	}
	
	var p = document.forms["myLogin"]["password"].value;
	if (p == null || p == "") {
		missing = 1;
	}

	if (missing) {	
		document.getElementById("formError").innerHTML = "Empty fields!";
		return false;
	}

	return true;
}
