#include "WaveformView.h"
#include "CaptureModel.h"

WaveformView::WaveformView() {
	WantFocus();
	Transparent();
}

void WaveformView::ZoomFit() {
	if(!model) return;
	pixelsPerUs = max(0.01, (double)GetWaveformArea().GetWidth() / (timeEnd - timeStart));
	Refresh();
}

Rect WaveformView::GetWaveformArea() const {
	Size sz = GetSize();
	return Rect(headerWidth, rulerHeight, sz.cx - headerWidth, sz.cy - rulerHeight);
}

Rect WaveformView::GetHeaderArea() const {
	Size sz = GetSize();
	return Rect(0, rulerHeight, headerWidth, sz.cy - rulerHeight);
}

Rect WaveformView::GetRulerArea() const {
	Size sz = GetSize();
	return Rect(headerWidth, 0, sz.cx - headerWidth, rulerHeight);
}

void WaveformView::Paint(Draw& w) {
	Size sz = GetSize();
	w.DrawRect(sz, White());
	
	Rect waveArea = GetWaveformArea();
	Rect headerArea = GetHeaderArea();
	Rect rulerArea = GetRulerArea();
	
	w.DrawRect(headerArea, Color(245, 245, 245));
	w.DrawRect(rulerArea, Color(240, 240, 240));
	w.DrawRect(waveArea, White());
	
	w.DrawLine(headerArea.right, headerArea.top, headerArea.right, headerArea.bottom, 1, Gray());
	w.DrawLine(rulerArea.left, rulerArea.bottom, rulerArea.right, rulerArea.bottom, 1, Gray());
	
	DrawHeader(w, headerArea);
	DrawRuler(w, rulerArea);
	DrawWaveform(w, waveArea);
	
	if(cursorTime >= 0) {
		int cx = TimeToX(cursorTime);
		if(cx >= waveArea.left && cx <= waveArea.right) {
			w.DrawLine(cx, waveArea.top, cx, waveArea.bottom, 1, Red());
		}
	}
	
	if(selectionStart >= 0 && selectionEnd >= 0) {
		int sx = TimeToX(min(selectionStart, selectionEnd));
		int ex = TimeToX(max(selectionStart, selectionEnd));
		Color selColor(0, 120, 215);
		Color blendColor(
			(selColor.GetR() * 60 + 255 * 196) / 256,
			(selColor.GetG() * 60 + 255 * 196) / 256,
			(selColor.GetB() * 60 + 255 * 196) / 256
		);
		w.DrawRect(Rect(sx, waveArea.top, ex - sx, waveArea.GetHeight()), blendColor);
	}
}

void WaveformView::DrawHeader(Draw& w, const Rect& r) {
	if(!model) return;
	
	const Array<Track>& tracks = model->GetTracks();
	int y = r.top;
	
	for(int i = 0; i < tracks.GetCount(); i++) {
		const Track& track = tracks[i];
		Rect tr(r.left, y, r.GetWidth(), channelHeight);
		
		if(y + channelHeight > r.bottom) break;
		
		if(i % 2 == 0) {
			w.DrawRect(tr, Color(250, 250, 250));
		}
		
		Color trackColor = track.visible ? track.color : Gray();
		w.DrawRect(Rect(tr.left + 4, tr.top + 8, 16, 16), trackColor);
		// Draw border as four lines
		w.DrawLine(tr.left + 4, tr.top + 8, tr.left + 19, tr.top + 8, 1, Gray());
		w.DrawLine(tr.left + 4, tr.top + 8, tr.left + 4, tr.top + 23, 1, Gray());
		w.DrawLine(tr.left + 19, tr.top + 8, tr.left + 19, tr.top + 23, 1, Gray());
		w.DrawLine(tr.left + 4, tr.top + 23, tr.left + 19, tr.top + 23, 1, Gray());
		
		String displayName = track.name;
		if(track.kind == TK_UART_RX) displayName += " (RX)";
		else if(track.kind == TK_UART_TX) displayName += " (TX)";
		else if(track.kind == TK_CUSTOM_BITS) displayName += " [Bits]";
		else if(track.kind == TK_CUSTOM_ADDR_DATA) displayName += " [Addr/Data]";
		
		w.DrawText(tr.left + 24, tr.top + 8, displayName, StdFont(), trackColor);
		
		if(!track.decodedBlocks.IsEmpty()) {
			w.DrawText(tr.left + 24, tr.top + 20, Format("%d blocks", track.decodedBlocks.GetCount()), StdFont(10), Gray());
		}
		
		y += channelHeight;
	}
}

void WaveformView::DrawRuler(Draw& w, const Rect& r) {
	if(!model) return;
	
	int64 duration = timeEnd - timeStart;
	if(duration <= 0) return;
	
	int majorStepUs = 1;
	while(duration / majorStepUs > 20) majorStepUs *= 10;
	while(duration / majorStepUs < 5) majorStepUs /= 10;
	
	int64 startMajor = (timeStart / majorStepUs) * majorStepUs;
	if(startMajor < timeStart) startMajor += majorStepUs;
	
	for(int64 t = startMajor; t <= timeEnd; t += majorStepUs) {
		int x = TimeToX(t);
		if(x < r.left || x > r.right) continue;
		
		w.DrawLine(x, r.top, x, r.bottom, 1, Gray());
		
		String label;
		if(t >= 1000000) label = Format("%.2fs", t / 1000000.0);
		else if(t >= 1000) label = Format("%.1fms", t / 1000.0);
		else label = Format("%dus", (int)t);
		
		Size txtSize = GetTextSize(label, StdFont(10));
		w.DrawText(x - txtSize.cx / 2, r.top + 2, label, StdFont(10), Black());
	}
}

void WaveformView::DrawWaveform(Draw& w, const Rect& r) {
	if(!model) return;
	
	const Array<Track>& tracks = model->GetTracks();
	int64 visStart = timeStart;
	int64 visEnd = timeEnd;
	
	for(int i = 0; i < tracks.GetCount(); i++) {
		int y = TrackToY(i);
		Rect tr(r.left, y, r.GetWidth(), channelHeight);
		
		if(tr.bottom < r.top || tr.top > r.bottom) continue;
		
		const Track& track = tracks[i];
		if(!track.visible) continue;
		
		DrawChannel(w, tr, i, visStart, visEnd);
		
		if(i < tracks.GetCount() - 1) {
			w.DrawLine(tr.left, tr.bottom, tr.right, tr.bottom, 1, Color(230, 230, 230));
		}
	}
}

void WaveformView::DrawChannel(Draw& w, const Rect& r, int trackIndex, int64 visStart, int64 visEnd) {
	if(!model) return;
	
	const Track& track = model->GetTrack(trackIndex);
	int midY = r.top + r.GetHeight() / 2;
	
	w.DrawLine(r.left, midY, r.right, midY, 1, Color(240, 240, 240));
	
	if(track.kind == TK_RAW || track.kind == TK_UART_RX || track.kind == TK_UART_TX) {
		DrawRawWaveform(w, r, track, visStart, visEnd);
	}
	
	if(showDecoded && !track.decodedBlocks.IsEmpty()) {
		DrawDecodedBlocks(w, r, track, visStart, visEnd);
	}
}

void WaveformView::DrawRawWaveform(Draw& w, const Rect& r, const Track& track, int64 visStart, int64 visEnd) {
	if(track.transitions.IsEmpty()) return;
	
	int midY = r.top + r.GetHeight() / 2;
	int amplitude = r.GetHeight() / 2 - 4;
	
	Color waveColor = track.color;
	
	int prevX = TimeToX(max((int64)0, track.transitions[0].time));
	bool prevVal = track.transitions[0].value;
	int prevY = midY - (prevVal ? amplitude : -amplitude);
	
	for(size_t i = 1; i < track.transitions.GetCount(); i++) {
		const Transition& tr = track.transitions[i];
		if(tr.time < visStart) continue;
		if(tr.time > visEnd) break;
		
		int x = TimeToX(tr.time);
		bool val = tr.value;
		int y = midY - (val ? amplitude : -amplitude);
		
		w.DrawLine(prevX, prevY, x, prevY, 2, waveColor);
		w.DrawLine(x, prevY, x, y, 2, waveColor);
		
		prevX = x;
		prevY = y;
		prevVal = val;
	}
	
	if(prevX < r.right) {
		w.DrawLine(prevX, prevY, r.right, prevY, 2, waveColor);
	}
}

void WaveformView::DrawDecodedBlocks(Draw& w, const Rect& r, const Track& track, int64 visStart, int64 visEnd) {
	int blockY = r.top + 4;
	int blockHeight = 18;
	
	for(const DecodedBlock& block : track.decodedBlocks) {
		if(block.endTime < visStart || block.startTime > visEnd) continue;
		
		int x1 = TimeToX(max(block.startTime, visStart));
		int x2 = TimeToX(min(block.endTime, visEnd));
		
		if(x2 <= x1) continue;
		
		Rect br(x1, blockY, x2 - x1, blockHeight);
		w.DrawRect(br, block.color);
		
		// Draw border as four lines
		Color borderColor(
			(block.color.GetR() * 80 + 0 * 176) / 256,
			(block.color.GetG() * 80 + 0 * 176) / 256,
			(block.color.GetB() * 80 + 0 * 176) / 256
		);
		w.DrawLine(br.left, br.top, br.right, br.top, 1, borderColor);
		w.DrawLine(br.left, br.top, br.left, br.bottom, 1, borderColor);
		w.DrawLine(br.right, br.top, br.right, br.bottom, 1, borderColor);
		w.DrawLine(br.left, br.bottom, br.right, br.bottom, 1, borderColor);
		
		Size txtSize = GetTextSize(block.text, StdFont(9));
		if(txtSize.cx < br.GetWidth() - 4) {
			w.DrawText(br.left + 2, br.top + 1, block.text, StdFont(9), White());
		}
		
		blockY += blockHeight + 2;
		if(blockY + blockHeight > r.bottom) break;
	}
}

int WaveformView::TimeToX(int64 t) const {
	return headerWidth + (int)((t - timeStart) * pixelsPerUs);
}

int64 WaveformView::XToTime(int x) const {
	return timeStart + (int64)((x - headerWidth) / pixelsPerUs);
}

int WaveformView::TrackToY(int trackIndex) const {
	return rulerHeight + trackIndex * channelHeight;
}

int WaveformView::TrackIndexFromY(int y) const {
	return (y - rulerHeight) / channelHeight;
}

void WaveformView::LeftDown(Point p, dword keyflags) {
	SetFocus();
	
	Rect waveArea = GetWaveformArea();
	if(!waveArea.Contains(p)) return;
	
	if(keyflags & K_SHIFT) {
		selectionStart = XToTime(p.x);
		selectionEnd = selectionStart;
	} else {
		panning = true;
		panStart = p;
		panTimeStart = timeStart;
		SetCapture();
	}
	
	cursorTime = XToTime(p.x);
	if(WhenCursorChange) WhenCursorChange();
	Refresh();
}

void WaveformView::LeftUp(Point p, dword keyflags) {
	if(panning) {
		panning = false;
		ReleaseCapture();
	}
}

void WaveformView::MouseMove(Point p, dword keyflags) {
	Rect waveArea = GetWaveformArea();
	if(!waveArea.Contains(p)) return;
	
	if(panning) {
		int dx = panStart.x - p.x;
		timeStart = panTimeStart + (int64)(dx / pixelsPerUs);
		timeEnd = timeStart + (int64)(waveArea.GetWidth() / pixelsPerUs);
		Refresh();
	} else if(keyflags & K_MOUSELEFT) {
		selectionEnd = XToTime(p.x);
		if(WhenSelectionChange) WhenSelectionChange();
		Refresh();
	} else {
		cursorTime = XToTime(p.x);
		if(WhenCursorChange) WhenCursorChange();
		Refresh();
	}
}

void WaveformView::MouseWheel(Point p, int zdelta, dword keyflags) {
	Rect waveArea = GetWaveformArea();
	if(!waveArea.Contains(p)) return;
	
	if(keyflags & K_CTRL) {
		double factor = zdelta > 0 ? 1.2 : 0.8333;
		int64 centerTime = XToTime(p.x);
		pixelsPerUs *= factor;
		timeStart = centerTime - (int64)((p.x - headerWidth) / pixelsPerUs);
		timeEnd = timeStart + (int64)(waveArea.GetWidth() / pixelsPerUs);
		Refresh();
	}
}

Image WaveformView::CursorImage(Point p, dword keyflags) {
	Rect waveArea = GetWaveformArea();
	if(waveArea.Contains(p)) {
		if(keyflags & K_MOUSELEFT) return Image();
		return Image();
	}
	return Image();
}

void WaveformView::Layout() {
	Ctrl::Layout();
}