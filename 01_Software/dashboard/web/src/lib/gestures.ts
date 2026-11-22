// Touch gestures of the pager:
//  - horizontal swipe between pages, following the finger;
//  - pull down from the top edge for the control bar, following the finger;
//  - swipe up for the overview, swipe down to close it;
//  - double tap on an empty spot toggles the overview.
// Pointer events cover touch on the Pi and the mouse on a PC. Only the outcome reaches the
// pager; it moves things with transform alone. Elements with data-noswipe (sliders) keep
// their drags. A second finger cancels the gesture.
import type { Attachment } from 'svelte/attachments';

export interface PagerHandlers {
  canSwipe(): boolean;
  /** The finger moved dx px horizontally since it went down. */
  onDrag(dx: number): void;
  /** The finger lifted after a swipe; vx is its last speed in px/ms (negative = leftwards). */
  onRelease(dx: number, vx: number): void;
  /** May a vertical drag that started at startY (moving by dy) pull the control bar? */
  canPull(startY: number, dy: number): boolean;
  /** The finger moved dy px vertically since it went down. */
  onPull(dy: number): void;
  /** The finger lifted after a pull; vy is its last speed in px/ms (positive = down). */
  onPullEnd(dy: number, vy: number): void;
  /** May a vertical swipe in this direction act (open or close the overview)? */
  canSwipeVertical(direction: 'up' | 'down'): boolean;
  onSwipeVertical(direction: 'up' | 'down'): void;
  onDoubleTap(): void;
}

const SWIPE_START_PX = 12;
const VERTICAL_SWIPE_PX = 90; // or a quick flick, see VERTICAL_FLICK
const VERTICAL_FLICK = 0.5; // px/ms
const DOUBLE_TAP_MS = 320;
const DOUBLE_TAP_PX = 40;

/** Elements that take taps themselves; a double tap there does not open the overview. */
const INTERACTIVE = 'button, a, input, [data-interactive]';

export function pagerGestures(h: PagerHandlers): Attachment<HTMLElement> {
  return (el) => {
    const points = new Set<number>();
    let mode: 'idle' | 'pending' | 'drag' | 'pull' | 'vswipe' | 'done' = 'idle';
    let startX = 0;
    let startY = 0;
    let lastX = 0;
    let lastY = 0;
    let lastT = 0;
    let vx = 0;
    let vy = 0;
    let lastTap = { t: 0, x: 0, y: 0 };
    let swallowClick = false;

    const down = (e: PointerEvent) => {
      points.add(e.pointerId);
      if (points.size > 1) {
        if (mode === 'drag') h.onRelease(0, 0);
        if (mode === 'pull') h.onPullEnd(0, 0);
        mode = 'done';
        return;
      }
      const own = e.target instanceof Element && e.target.closest('[data-noswipe]');
      mode = own ? 'done' : 'pending';
      startX = lastX = e.clientX;
      startY = lastY = e.clientY;
      lastT = e.timeStamp;
      vx = 0;
      vy = 0;
      swallowClick = false;
    };

    const track = (e: PointerEvent) => {
      const dt = e.timeStamp - lastT;
      if (dt > 0) {
        vx = 0.7 * ((e.clientX - lastX) / dt) + 0.3 * vx;
        vy = 0.7 * ((e.clientY - lastY) / dt) + 0.3 * vy;
      }
      lastX = e.clientX;
      lastY = e.clientY;
      lastT = e.timeStamp;
    };

    const move = (e: PointerEvent) => {
      if (!points.has(e.pointerId)) return;
      if (mode === 'pending') {
        const dx = e.clientX - startX;
        const dy = e.clientY - startY;
        const horizontal = Math.abs(dx) > SWIPE_START_PX && Math.abs(dx) > Math.abs(dy) * 1.2;
        const vertical = Math.abs(dy) > SWIPE_START_PX && Math.abs(dy) > Math.abs(dx) * 1.2;
        if (horizontal && h.canSwipe()) {
          mode = 'drag';
        } else if (vertical && h.canPull(startY, dy)) {
          mode = 'pull';
        } else if (vertical && h.canSwipeVertical(dy < 0 ? 'up' : 'down')) {
          mode = 'vswipe';
        } else if (Math.hypot(dx, dy) > SWIPE_START_PX * 2) {
          mode = 'done'; // some other movement: neither a tap nor one of our gestures
        }
        if (mode !== 'pending') swallowClick = true;
      }
      track(e);
      if (mode === 'drag') h.onDrag(e.clientX - startX);
      else if (mode === 'pull') h.onPull(e.clientY - startY);
    };

    const up = (e: PointerEvent) => {
      if (!points.delete(e.pointerId)) return;
      if (points.size > 0) return;
      const dy = e.clientY - startY;
      if (mode === 'drag') {
        h.onRelease(e.clientX - startX, vx);
      } else if (mode === 'pull') {
        h.onPullEnd(dy, vy);
      } else if (mode === 'vswipe') {
        if (Math.abs(dy) > VERTICAL_SWIPE_PX || Math.abs(vy) > VERTICAL_FLICK) {
          h.onSwipeVertical(dy < 0 ? 'up' : 'down');
        }
      } else if (mode === 'pending' && e.type === 'pointerup') {
        const onControl = e.target instanceof Element && e.target.closest(INTERACTIVE);
        if (!onControl) {
          const near = Math.hypot(e.clientX - lastTap.x, e.clientY - lastTap.y) < DOUBLE_TAP_PX;
          if (e.timeStamp - lastTap.t < DOUBLE_TAP_MS && near) {
            lastTap.t = 0;
            // The click of this second tap would land on whatever the double tap just opened.
            swallowClick = true;
            h.onDoubleTap();
          } else {
            lastTap = { t: e.timeStamp, x: e.clientX, y: e.clientY };
          }
        }
      }
      mode = 'idle';
    };

    // A gesture must not also press the button the finger started or ended on.
    const click = (e: MouseEvent) => {
      if (swallowClick) {
        e.stopPropagation();
        e.preventDefault();
        swallowClick = false;
      }
    };

    el.addEventListener('pointerdown', down);
    el.addEventListener('pointermove', move);
    el.addEventListener('pointerup', up);
    el.addEventListener('pointercancel', up);
    el.addEventListener('click', click, true);
    return () => {
      el.removeEventListener('pointerdown', down);
      el.removeEventListener('pointermove', move);
      el.removeEventListener('pointerup', up);
      el.removeEventListener('pointercancel', up);
      el.removeEventListener('click', click, true);
    };
  };
}
