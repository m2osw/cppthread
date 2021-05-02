<?php
# Display this directory to users
# (code used to display test results)
#
# Copyright (c) 2006-2021  Made to Order Software Corp.  All Rights Reserved
#
# https://snapwebsites.org/project/cppthread
# contact@m2osw.com
# 
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

$dir = glob("cppthread*");

echo "<html><head><title>cppthread coverage</title></head>";
echo "<body><h1>cppthread coverage</h1><table border=\"1\" cellpadding=\"10\" cellspacing=\"0\"><tbody><tr><th>Coverage</th></tr>";
foreach($dir as $d)
{
    echo "<tr>";

    echo "<td><a href=\"", $d, "\">", $d, "</a></td>";

    echo "</tr>";
}
echo "</tbody></table></body></html>";

# vim: ts=4 sw=4 et
