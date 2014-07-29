/*
    Copyright (C) 2014 Waves Audio Ltd.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "ardour/meter.h"
#include "ardour/track.h"
#include "ardour_ui.h"
#include "gui_thread.h"
#include "meter_patterns.h"
#include "compact_meter_strip.h"

#include "dbg_msg.h"

using namespace ARDOUR;
using namespace PBD;
using namespace Gtk;
using namespace Gtkmm2ext;
using namespace std;
using namespace ArdourMeter;

PBD::Signal1<void,CompactMeterStrip*> CompactMeterStrip::CatchDeletion;
int CompactMeterStrip::__meter_width = 4;

CompactMeterStrip::CompactMeterStrip (Session* sess, boost::shared_ptr<ARDOUR::Route> rt)
	: EventBox()
	, WavesUI ("compact_meter_strip.xml", *this)
	, _route(rt)
	, _level_meter_home (get_box ("level_meter_home"))
	, _level_meter (sess)
	, _record_indicator (get_event_box ("record_indicator"))
	, _serial_number (0)
{
	set_attributes (*this, *xml_tree ()->root (), XMLNodeMap ());

	_level_meter.set_meter (_route->shared_peak_meter().get());
	_level_meter.clear_meters();
	_level_meter.set_type (_route->meter_type());
	_level_meter.setup_meters (__meter_width, __meter_width);
	_level_meter_home.add (_level_meter);

	_route->shared_peak_meter()->ConfigurationChanged.connect (_route_connections,
		                                                       invalidator (*this),
															   boost::bind (&CompactMeterStrip::meter_configuration_changed, 
															                this,
																			_1), 
															   gui_context());

	meter_configuration_changed (_route->shared_peak_meter()->input_streams ());

	_route->DropReferences.connect (_route_connections,
									invalidator (*this),
									boost::bind (&CompactMeterStrip::self_delete,
												 this),
									gui_context());
	boost::shared_ptr<Track> t = boost::dynamic_pointer_cast<Track> (_route);
	if (t) {
		t->RecordEnableChanged.connect (_route_connections,
										invalidator (*this),
										boost::bind (&CompactMeterStrip::update_rec_display,
													 this), gui_context());
		update_rec_display ();
	}
}

CompactMeterStrip::~CompactMeterStrip ()
{
	CatchDeletion (this);
}

void
CompactMeterStrip::self_delete ()
{
	delete this;
}

void
CompactMeterStrip::update_rec_display ()
{
	_record_indicator.set_state ((_route && _route->record_enabled ()) ? Gtk::STATE_ACTIVE : Gtk::STATE_NORMAL);
}

void
CompactMeterStrip::fast_update ()
{
	_level_meter.update_meters();
}

void
CompactMeterStrip::meter_configuration_changed (ChanCount c)
{
	_level_meter.setup_meters (__meter_width, __meter_width);
}

