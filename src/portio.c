/*
*  C Implementation: portio
*
* Description: 
* A wrapper around the serial interface (termios)
*
* Author: Ing. Gerard van der Sel
*
* Copyright: See COPYING file that comes with this distribution
*
* Information about configuring the Comport is found on:
* Serial programming HOWTO:
* http://www.tldp.org/HOWTO/Serial-Programming-HOWTO/index.html
* Test for serial transmission and reception for thread and interrupt control
* http://www.easysw.com/~mike/serial
*/

#include <stdincludes.h>

#include "config-srcpd.h"
#include "ttycygwin.h"
#include "portio.h"
/**
 * open_port:
 * Open and initialise the serial port
 * Port is opened for read/write
 *         8 bits and 1 stop bit, no parity.
 *         No timeout at reading the port
 * Input: Number of the bus
 * Output: >=0 a device is successfully attached
 *          <0 a error is reported.
 */
int open_port(bus_t bus)
{
	int serial;
	struct termios settings;

	serial  = open(busses[bus].filename.path, O_RDWR | O_NOCTTY);
	if (serial < 0)
	{
		DBG(bus, DBG_ERROR, "Error, could not open %s.\n Reported error number: %d.\n",
			busses[bus].filename.path, errno); // , str_errno(errno));
		busses[bus].fd = -1;
		return -errno;
	}
	/* Get default settings from OS */
	tcgetattr(serial, &busses[bus].devicesettings);
	/* Ignore default setting from the OS */
	bzero(&settings, sizeof(struct termios));
	/* Setup settings for serial port */
	/* Baud rate, enable reading, local mode, 8 bits, 1 stop bit and no parity */
	settings.c_cflag = busses[bus].baudrate | CREAD | CLOCAL | CS8;
	/* No parity control or generation */
	settings.c_iflag = IGNPAR;
	settings.c_oflag = 0;
	settings.c_lflag = 0;
	settings.c_cc[VMIN] = 1;      /* Block terminates with reception of 1 character */
	settings.c_cc[VTIME] = 0;     /* never a timeout */
	/* Apply baud rate (new style)*/
	cfsetospeed(&settings, busses[bus].baudrate);
	cfsetispeed(&settings, busses[bus].baudrate);
	/* Apply settings for serial port */
	tcsetattr(serial, TCSANOW, &settings);
	/* Flush serial buffers                                                          */
	tcflush(serial, TCIFLUSH);
	tcflush(serial, TCOFLUSH);
	busses[bus].fd = serial;
	return serial;
}

/**
 * close_port:
 * closes a comport and restores its old settings
 */
void close_port(bus_t bus)
{
	tcsetattr(busses[bus].fd, TCSANOW, &busses[bus].devicesettings);
	close(busses[bus].fd);
	DBG(bus, DBG_ERROR, "Port %s is closed.\n", busses[bus].filename.path);
	busses[bus].fd = -1;
}

/**
 * write_port
 */
void write_port(bus_t bus, unsigned char b)
{
	int i;

	i = 0;
	if (busses[bus].debuglevel <= DBG_DEBUG)
	{
		/* Wait for transmit queue to go empty */
		tcdrain(busses[bus].fd);
		i = write(busses[bus].fd, &b, 1);
	}
	if (i < 0)
	{
		/* Error reported from write */
		DBG(bus, DBG_ERROR, "(FD: %d) External error: errno %d",
			busses[bus].fd, errno); // , str_errno(errno));
	}
	else
	{
		/* Character transmitted */
		DBG(bus, DBG_DEBUG, "(FD: %d) %i byte sent: 0x%02x (%d)",
			busses[bus].fd, i, b, b);
	}
}

/**
 * read_port
 */
int read_port(bus_t bus,  unsigned char *rr)
{
	int i;

	// with debug level beyond DBG_DEBUG, we will not really work on hardware
	if (busses[bus].debuglevel > DBG_DEBUG)
	{
		i = 1;
		*rr = 0;
	}
	else
	{
		// read, wait for an input
		i = read(busses[bus].fd, rr, 1);
		if (i < 0)
		{
			char emsg[200];
			strerror_r(errno, emsg, sizeof(emsg));
			DBG(bus, DBG_ERROR,
				"read_comport(): read status: %d with errno = %d: %s.\n",
				i, errno, *emsg);
		}
		if (i > 0)
		{
			DBG(bus, DBG_DEBUG, "read_comport(): byte read: 0x%02x.\n",
				*rr);
		}
	}
	return (i > 0 ? 0 : -1);
}


/**
 * check_port
 */
int check_port(bus_t bus)
{
	int temp;

	ioctl(busses[bus].fd, FIONREAD, &temp);
	return (temp > 0 ? -1 : 0);
}
