// One shared "now" for every component, updated on each full minute.
import { minuteClock } from './format';

export const clock = $state({ now: new Date() });

minuteClock((now) => (clock.now = now));
