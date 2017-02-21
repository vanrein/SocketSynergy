/* The daemon interface to the synergy functionality.  When the library detects
 * that it is not running as root, it will forward a request to this daemon
 * over a connection-less UNIX domain socket.  The request will be handled
 * immediately upon arrival, as an asynchronous service to the requesting
 * process.  This does only the work that requires root privileges, namely
 * sending a RAW packet with manually crafted content.
 *
 * From: Rick van Rein <rick@openfortress.nl>
 */


#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#include <string.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <sys/socketsynergy.h>


void cleanup_socket (void) {
	unlink (SYNERGY_DAEMON_SOCKET_PATH);
}

void cleanup_pidfile (void) {
	unlink (SYNERGY_DAEMON_PID_FILE);
}


int main (int argc, char *argv []) {
	uint8_t minhoplim = 1;
	uint8_t maxhoplim = 254;
	//
	// Sanity checks
	if (argc > 3) {
		fprintf (stderr, "USAGE: %s [minhoplimit [maxhoplimit]]\n",
				argv [0]);
		exit (1);
	}
	if (argc > 1) {
		minhoplim = atoi (argv [1]);
	}
	if (argc > 2) {
		maxhoplim = atoi (argv [2]);
	}
	if (minhoplim < 1) {
		fprintf (stderr, "SILLY: minhoplimit set to 1\n");
		minhoplim = 1;
	}
	if (maxhoplim > 254) {
		fprintf (stderr, "SILLY: maxhoplimit set to 254\n");
		maxhoplim = 254;
	}
	if (minhoplim > maxhoplim) {
		fprintf (stderr, "SILLY: maxhoplimit set to minhoplimit\n");
		maxhoplim = minhoplim;
	}
	if (geteuid () != 0) {
		fprintf (stderr, "FATAL: %s must run with effective UID 0\n",
			argv [0]);
		exit (1);
	}
	//
	// Check if the daemon is already running
	int pidf = open (SYNERGY_DAEMON_PID_FILE, O_RDONLY);
	if (pidf >= 0) {
		char buf [10];
		ssize_t buflen = read (pidf, buf, sizeof (buf));
		if ((buflen < 0) || (buflen > sizeof (buf)-1)) {
			fprintf (stderr, "Unexpected length in pidfile\n");
			exit (1);
		}
		buf [buflen] = '\0';
		pid_t pid = atoi (buf);
		if (pid == 0) {
			fprintf (stderr, "Unexpected contents in pidfile\n");
			exit (1);
		}
		if ((kill (pid, 0) == -1) && (errno == ESRCH)) {
			/* Looks like a crashed daemon; cleanup after it */
			cleanup_socket ();
			cleanup_pidfile ();
		}
	}
	//
	// Open socket
	int sox = socket (PF_UNIX, SOCK_DGRAM, 0);
	if (sox == -1) {
		perror ("Could not obtain a socket for synergy.d");
		exit (1);
	}
	struct sockaddr_un socket_path;
	memset (&socket_path, 0, sizeof (socket_path));
	socket_path.sun_family = PF_UNIX;
	strncpy (socket_path.sun_path, SYNERGY_DAEMON_SOCKET_PATH,
				sizeof (socket_path.sun_path));
	if (bind (sox, (struct sockaddr *) &socket_path,
				sizeof (socket_path)) == -1) {
		if (errno == EADDRINUSE) {
			fprintf (stderr, "FATAL: An instance of %s already seems to be running\n", argv [0]);
		} else {
			perror ("Could not bind to synergy.d socket path");
		}
		exit (1);
	}
	//
	// Fork daemon process / cleanup and exit master
	pid_t child = fork ();
	switch (child) {
	case -1:
		/* Failed to fork; report and exit in agony */
		perror ("Failed to fork synergy.d daemon process");
		close (sox);
		cleanup_socket ();
		exit (1);
	default:
		/* Parent with successful child forked; cleanup and exit */
		fprintf (stderr, "Successfully forked synergy.d child process %d\n", child);
		close (sox);
		exit (0);
	case 0:
		/* Child was forked properly, detach and continue below */
		atexit (cleanup_socket);
		child = getpid ();
		char buf [10];
		snprintf (buf, sizeof (buf), "%d\n", child);
		pidf = open (SYNERGY_DAEMON_PID_FILE, O_WRONLY | O_CREAT | O_TRUNC);
		if (pidf == -1) {
			perror ("Child failed to write pidfile");
			close (sox);
			cleanup_socket ();
			exit (1);
		}
		atexit (cleanup_pidfile);
		write (pidf, buf, strlen (buf));
		close (pidf);
		setsid ();
		break;
	}
	//
	// Prepare for nice cleanup
	//
	// Run the service loop forever and ever
	char anc [CMSG_SPACE (sizeof (int))];
	struct synergy_request_message req;
	struct iovec iov;
	struct msghdr mgh;
handler_init:
	memset (&anc, 0, sizeof (anc));
	memset (&req, 0, sizeof (req));
	memset (&iov, 0, sizeof (iov));
	memset (&mgh, 0, sizeof (mgh));
	iov.iov_len = sizeof (req);
	iov.iov_base = &req;
	mgh.msg_iovlen = 1;
	mgh.msg_iov = &iov;
	mgh.msg_controllen = sizeof (anc);
	mgh.msg_control = &anc;
	ssize_t len;
	int todo = -1;
handler_loop:
	//
	// Close any open socket in todo -- we got it as a duplicate file handle
	if (todo >= 0) {
		close (todo);
		todo = -1;
	}
	//
	// Read a new message (block on it, it's all we do anyway)
	len = recvmsg (sox, &mgh, 0);
	if (len != sizeof (req)) {
		goto handler_loop;
	}
	//
	// Parse the message, validate its structure
	struct cmsghdr *cmg;
	cmg = CMSG_FIRSTHDR (&mgh);
	if (cmg->cmsg_len != CMSG_LEN (sizeof (int))) {
		goto handler_loop;
	}
	if ((cmg->cmsg_level != SOL_SOCKET) || (cmg->cmsg_type != SCM_RIGHTS)) {
		goto handler_loop;
	}
	todo = * (int *) CMSG_DATA (cmg);
	if (todo < 0) {
		goto handler_loop;
	}
	//
	// Apply minhoplim and maxhoplim
	if (req.hoplimit < minhoplim) {
		req.hoplimit = minhoplim;
	} else if (req.hoplimit > maxhoplim) {
		req.hoplimit = maxhoplim;
	}
	//
	// Invoke the synergy library operation with our root privileges
	if (synergy_privileged (todo, req.hoplimit, &req.symcli) != 0) {
		perror ("Privileged synergy operation failed");
		goto handler_loop;
	}
	//
	// Report positively
	fprintf (stderr, "Succeeded synergy operation\n");
	//
	// Continue the service loop
	goto handler_loop;
}
