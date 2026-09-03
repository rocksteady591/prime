export interface Message {
  id?: string;
  sender: string;
  text: string;
  timestamp: number;
  recipient?: string;
  status?: 'pending' | 'sent' | 'delivered' | 'error';
}