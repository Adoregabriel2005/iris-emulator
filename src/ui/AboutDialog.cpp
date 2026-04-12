#include "AboutDialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTabWidget>
#include <QLabel>
#include <QTextBrowser>
#include <QDialogButtonBox>
#include <QPixmap>
#include <QFont>

AboutDialog::AboutDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("About Íris Emulator"));
    setMinimumSize(600, 480);
    resize(640, 520);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    auto* root = new QVBoxLayout(this);

    // ── Header ────────────────────────────────────────────────────────────────
    auto* header = new QHBoxLayout();
    auto* logo = new QLabel(this);
    QPixmap px(":/iris_icon.svg");
    if (!px.isNull())
        logo->setPixmap(px.scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    logo->setFixedSize(72, 72);
    logo->setAlignment(Qt::AlignCenter);
    header->addWidget(logo);

    auto* titleCol = new QVBoxLayout();
    auto* titleLbl = new QLabel(tr("<h2>Íris Emulator</h2>"), this);
    titleLbl->setTextFormat(Qt::RichText);
    auto* verLbl = new QLabel(
        tr("<span style='color:#aaa;'>Version 1.17 &nbsp;·&nbsp; "
           "GPL-3.0 Open Source &nbsp;·&nbsp; Windows 10/11 64-bit</span>"), this);
    verLbl->setTextFormat(Qt::RichText);
    auto* devLbl = new QLabel(
        tr("<span style='color:#888;'>Developed by <b>Gorigamia</b> &amp; <b>Aleister93MarkV</b></span>"), this);
    devLbl->setTextFormat(Qt::RichText);
    titleCol->addWidget(titleLbl);
    titleCol->addWidget(verLbl);
    titleCol->addWidget(devLbl);
    titleCol->addStretch();
    header->addLayout(titleCol, 1);
    root->addLayout(header);

    auto* sep = new QFrame(this);
    sep->setFrameShape(QFrame::HLine);
    sep->setFrameShadow(QFrame::Sunken);
    sep->setStyleSheet("QFrame { color: rgba(255,255,255,30); }");
    root->addWidget(sep);

    // ── Tabs ──────────────────────────────────────────────────────────────────
    auto* tabs = new QTabWidget(this);
    tabs->setDocumentMode(true);

    // ── Tab: About ────────────────────────────────────────────────────────────
    auto* aboutBrowser = new QTextBrowser(this);
    aboutBrowser->setOpenExternalLinks(true);
    aboutBrowser->setFrameShape(QFrame::NoFrame);
    aboutBrowser->setHtml(tr(R"(
<style>
  body  { font-size:13px; }
  h3    { color:#5b9bd5; margin-bottom:4px; }
  p     { margin:4px 0 10px 0; }
  code  { background:#2a2a2a; padding:1px 4px; border-radius:3px; }
</style>

<h3>What is Íris?</h3>
<p>
Íris is an open source multi-system Atari emulator that runs
<b>Atari 2600</b>, <b>Atari Lynx</b>, and <b>Atari Jaguar</b> games
on modern Windows hardware.<br>
The interface is inspired by <b>PCSX2</b>, <b>DuckStation</b>, and
<b>Dolphin Emulator</b> — clean, professional, and easy to use.
</p>

<h3>Technology</h3>
<p>
Built with <b>C++17</b> · <b>Qt 6</b> · <b>SDL2</b> · <b>CMake</b><br>
Lynx core powered by <b>Gearlynx</b> (Ignacio Sanchez / drhelius)<br>
Jaguar core powered by <b>Virtual Jaguar</b> (Shamus / James L. Hammons)
</p>

<h3>Console Languages / Hardware</h3>
<p>
<b>Atari 2600</b> — MOS 6507 CPU (1.19 MHz) · TIA · RIOT 6532<br>
<b>Atari Lynx</b> — WDC 65C02 CPU (4 MHz) · Mikey · Suzy<br>
<b>Atari Jaguar</b> — Motorola 68000 + Tom + Jerry (26.59 MHz RISC)
</p>

<h3>Inspiration</h3>
<p>
Interface design inspired by <b>PCSX2</b>, <b>DuckStation</b>, and <b>Dolphin Emulator</b>.<br>
2600 documentation from the <b>Stella</b> team and the <b>AtariAge</b> community.
</p>
)"));
    tabs->addTab(aboutBrowser, tr("About"));

    // ── Tab: Credits ─────────────────────────────────────────────────────────
    auto* creditsBrowser = new QTextBrowser(this);
    creditsBrowser->setOpenExternalLinks(true);
    creditsBrowser->setFrameShape(QFrame::NoFrame);
    creditsBrowser->setHtml(tr(R"(
<style>
  body { font-size:13px; }
  h3   { color:#5b9bd5; margin-bottom:4px; }
  p    { margin:4px 0 10px 0; }
  ul   { margin:2px 0 10px 16px; }
</style>

<h3>Developers</h3>
<ul>
  <li><b>Gorigamia</b> — Lead developer, Atari 2600 core, UI, architecture</li>
  <li><b>Aleister93MarkV</b> — Co-developer, testing, contributions</li>
</ul>

<h3>Atari 2600 — Hardware History</h3>
<p>
The <b>Atari 2600</b> (originally Atari VCS) was developed by <b>Atari, Inc.</b>,
under the leadership of engineer <b>Jay Miner</b> — who later went on to design
the Amiga chipset. Released in 1977, it became one of the best-selling consoles
of all time.
</p>

<h3>Atari Lynx — Hardware History</h3>
<p>
The <b>Atari Lynx</b> was originally developed by the software company <b>Epyx</b>
in 1987, initially named <i>"Handy Game"</i>. The principal designers were
<b>Dave Needle</b> and <b>R.J. Mical</b>, known for their work on the Amiga computer.
Due to financial difficulties at Epyx, <b>Atari Corp.</b> took over production
and launched the Lynx in 1989 — making it the world's first color handheld console.
</p>

<h3>Atari Jaguar — Hardware History</h3>
<p>
The <b>Atari Jaguar</b> was developed primarily by <b>Flare Technology</b>
(Martin Brennan and John Mathieson) in partnership with Atari Corp.
Released in 1993, it was marketed as the first 64-bit home console.
</p>

<h3>Third-Party Engines &amp; Libraries</h3>
<ul>
  <li><b>Gearlynx</b> by <a href='https://github.com/drhelius/Gearlynx'>Ignacio Sanchez (drhelius)</a> — Atari Lynx core</li>
  <li><b>Virtual Jaguar</b> by Shamus / James L. Hammons — Atari Jaguar core</li>
  <li><b>Stella</b> team &amp; <b>AtariAge</b> community — 2600 documentation</li>
  <li><b>Qt 6</b> — UI framework</li>
  <li><b>SDL2</b> — Audio &amp; input</li>
  <li><b>VST3 SDK</b> by Steinberg — Audio plugin support</li>
  <li><b>discord-rpc</b> — Discord Rich Presence</li>
</ul>

<h3>Interface Inspiration</h3>
<ul>
  <li><b>PCSX2</b> — Overall layout, settings dialog structure, game list</li>
  <li><b>DuckStation</b> — Toolbar style, overlay menu</li>
  <li><b>Dolphin Emulator</b> — Save state slots, cover art grid</li>
</ul>
)"));
    tabs->addTab(creditsBrowser, tr("Credits"));

    // ── Tab: License ─────────────────────────────────────────────────────────
    auto* licenseBrowser = new QTextBrowser(this);
    licenseBrowser->setOpenExternalLinks(true);
    licenseBrowser->setFrameShape(QFrame::NoFrame);
    licenseBrowser->setHtml(tr(R"(
<style>
  body { font-size:12px; font-family: monospace; }
  h3   { color:#5b9bd5; font-family: sans-serif; }
  p    { margin: 6px 0; }
</style>

<h3>GNU General Public License v3.0</h3>

<p>Íris Emulator is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.</p>

<p>This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.</p>

<p>You should have received a copy of the GNU General Public License
along with this program. If not, see
<a href='https://www.gnu.org/licenses/'>https://www.gnu.org/licenses/</a>.</p>

<p>The <b>Gearlynx</b> engine is copyright &copy; 2025 Ignacio Sanchez,
also licensed under GPL-3.0.</p>

<p>The <b>Virtual Jaguar</b> core is copyright &copy; its respective authors,
also licensed under GPL-3.0.</p>

<p>The <b>VST3 SDK</b> is copyright &copy; Steinberg Media Technologies GmbH,
used under the VST3 SDK License Agreement.</p>

<p>ROMs and BIOS files are <b>NOT included</b> and cannot be provided.
You must own the original hardware to legally use ROM files.</p>
)"));
    tabs->addTab(licenseBrowser, tr("License"));

    root->addWidget(tabs, 1);

    // ── Close button ─────────────────────────────────────────────────────────
    auto* btnBox = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(btnBox, &QDialogButtonBox::rejected, this, &QDialog::accept);
    root->addWidget(btnBox);
}
